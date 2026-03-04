import torch
import torch.nn as nn
from torchvision import datasets
from common.transform.minst_transform import ResizeDeskewTransform
from common.transform.soft_threshold import SoftThreshold
from spikingjelly.activation_based import neuron, surrogate, functional
from spikingjelly.activation_based import encoding
from common.get_torch_device import get_torch_device
from common.get_mnist_loaders import get_mnist_loaders
from common.nn_model.train_model import train_model, standard_epoch_log_fn
from common.util.output_path import output_path
from common.util.plot_curve import plot_curve
from common.nn_odin.odin_linear import ODIN_Linear
from common.nn_odin.odin_lif_node import ODIN_LIFNode
import optuna
from optuna.trial import TrialState
import optuna.visualization.matplotlib as plot
from optuna.storages import JournalStorage
from optuna.storages.journal import JournalFileBackend
import json
import os


class SNN(nn.Module):
    def __init__(self, timesteps: int, soft_threshold: float, init_parameters):
        super().__init__()
        self.timesteps = timesteps 
        self.soft_threshold = soft_threshold
        self.encoder = encoding.PoissonEncoder(step_mode='m')
        
        self.sections = nn.ModuleList([
            nn.Sequential(
                ODIN_Linear(14*14, 64),
                ODIN_LIFNode(
                    n_neurons=64,
                    init_v_thr=init_parameters['section_v_thr'],
                    init_leak_str=init_parameters['section_leak_str']
                )
            )
            for _ in range(4)
        ])
        self.fusion = nn.Sequential(
            ODIN_Linear(4*64, 10),
            ODIN_LIFNode(
                n_neurons=10,
                init_v_thr=init_parameters['fusion_v_thr'],
                init_leak_str=init_parameters['fusion_leak_str']
            )
        )
        functional.set_step_mode(self, step_mode='m')

    def split_into_4_sections(self, x):
        s00 = x[:, 0::2, 0::2]  # x%2==0, y%2==0
        s10 = x[:, 1::2, 0::2]  # x%2==1, y%2==0
        s01 = x[:, 0::2, 1::2]  # x%2==0, y%2==1
        s11 = x[:, 1::2, 1::2]  # x%2==1, y%2==1
        return [
            s00.flatten(1),
            s10.flatten(1),
            s01.flatten(1),
            s11.flatten(1),
        ]

    def forward(self, x):
        x = SoftThreshold(threshold=self.soft_threshold)(x)
        
        x = x.squeeze(1)
        section_inputs = self.split_into_4_sections(x)
        
        section_inputs = [
            s.unsqueeze(0).repeat(self.timesteps, 1, 1)
            for s in section_inputs 
        ]
        section_inputs = [
            self.encoder(s) 
            for s in section_inputs 
        ]

        self.batch_spike_count = sum(s.sum().item() for s in section_inputs)

        section_outputs = []
        for input, net in zip(section_inputs, self.sections):
            out = net(input)
            section_outputs.append(out)

        fused = torch.cat(section_outputs, dim=2)
        self.batch_spike_count += fused.sum().item()

        out_spikes = self.fusion(fused)  
        self.batch_spike_count += out_spikes.sum().item()
        firing_rate = out_spikes.sum(dim=0)
        functional.reset_net(self)
        return firing_rate

def make_objective(device, train_loader, val_loader, batch_size, num_epochs):
    def objective(trial):
        timesteps = trial.suggest_int("timesteps", 1, 16) # 16
        soft_threshold = trial.suggest_float("soft_threshold", 0.0, 0.75)
        section_v_thr = trial.suggest_int("section_v_thr", 1, 2 ** 6 - 1)
        section_leak_str = trial.suggest_int("section_leak_str", 0, section_v_thr)
        fusion_v_thr = trial.suggest_int("fusion_v_thr", 1, 2 ** 6 - 1)
        fusion_leak_str = trial.suggest_int("fusion_leak_str", 0, fusion_v_thr)
        init_parameters = {
            'section_v_thr': float(section_v_thr),
            'section_leak_str': float(section_leak_str),
            'fusion_v_thr': float(fusion_v_thr),
            'fusion_leak_str': float(fusion_leak_str),
        }

        print(f"Trial params: timesteps={timesteps}, soft_threshold={soft_threshold}, init_parameters={init_parameters}")

        model = SNN(
            timesteps=timesteps, 
            soft_threshold=soft_threshold, 
            init_parameters=init_parameters
            ).to(device)
        criterion = nn.CrossEntropyLoss()
        # optimizer = torch.optim.Adam(model.parameters(), lr=0.05)
        optimizer = torch.optim.Adam(model.parameters(), lr=0.05)
        scheduler = torch.optim.lr_scheduler.CosineAnnealingLR(optimizer, T_max=num_epochs)

        spike_count_log = []
        def epoch_log_fn(model, epoch, num_epochs, train_loss, train_acc, val_loss, val_acc):
            spike_count_log.append(model.batch_spike_count / batch_size)
            standard_epoch_log_fn(model, epoch, num_epochs, train_loss, train_acc, val_loss, val_acc)
            for i, section in enumerate(model.sections):
                print(f"Section {i}:(v_thr: {section[1].v_thr_q.tolist()},\nleak_str: {section[1].leak_str_q.tolist()})")
            print(f"Fusion:(v_thr: {model.fusion[1].v_thr_q.tolist()},\nleak_str: {model.fusion[1].leak_str_q.tolist()})")
            if epoch > 5 and val_acc < 0.2:
                raise optuna.exceptions.TrialPruned()
            if epoch > 15 and val_acc < 0.6:
                raise optuna.exceptions.TrialPruned()

        train_logs = train_model(
            device=device, 
            model=model,
            criterion=criterion, 
            optimizer=optimizer, 
            scheduler=scheduler,
            train_loader=train_loader, 
            val_loader=val_loader,
            num_epochs=num_epochs,
            epoch_log_fn=epoch_log_fn
            )
        
        try:
            sections_list = []
            for sec in model.sections:
                linear = sec[0].weight_q.detach().cpu().tolist()
                v_thr = sec[1].v_thr_q.detach().cpu().tolist()
                leak = sec[1].leak_str_q.detach().cpu().tolist()
                sections_list.append({
                    'linear_weight': linear,
                    'v_thr_q': v_thr,
                    'leak_str_q': leak
                })

            fusion_dict = {
                'linear_weight': model.fusion[0].weight_q.detach().cpu().tolist(),
                'v_thr_q': model.fusion[1].v_thr_q.detach().cpu().tolist(),
                'leak_str_q': model.fusion[1].leak_str_q.detach().cpu().tolist()
            }

            model_dict = {
                'timesteps': timesteps,
                'soft_threshold': soft_threshold,
                'sections': sections_list,
                'fusion': fusion_dict
            }

            json_path = output_path(f"trials/snn_trial_{trial.number}.json")
            os.makedirs(os.path.dirname(json_path), exist_ok=True)
            with open(json_path, 'w', encoding='utf-8') as jf:
                json.dump(model_dict, jf, indent=2, ensure_ascii=False)

            trial.set_user_attr('sections', [{'v_thr_q': s['v_thr_q'], 'leak_str_q': s['leak_str_q']} for s in sections_list])
            trial.set_user_attr('fusion', {'v_thr_q': fusion_dict['v_thr_q'], 'leak_str_q': fusion_dict['leak_str_q']})
        except Exception as e:
            print(f"Warning: failed to save model JSON for trial {trial.number}: {e}")

        return train_logs['top_val_acc'], spike_count_log[train_logs['top_epoch']]
    return objective

if __name__ == "__main__":
    batch_size = 1024 * 4
    num_epochs = 30 
    n_trials = 50
    
    transform = ResizeDeskewTransform(resize=(28, 28))
    train_loader, val_loader = get_mnist_loaders(batch_size=batch_size, transform=transform)
    
    device = get_torch_device()
    
    objective = make_objective(device, train_loader, val_loader, batch_size, num_epochs)
    study = optuna.create_study(directions=["maximize", "minimize"])
    study.optimize(objective, n_trials=n_trials)
    
    pruned_trials = study.get_trials(deepcopy=False, states=[TrialState.PRUNED])
    complete_trials = study.get_trials(deepcopy=False, states=[TrialState.COMPLETE])
    print("Study statistics: ")
    print("  Number of finished trials: ", len(study.trials))
    print("  Number of pruned trials: ", len(pruned_trials))
    print("  Number of complete trials: ", len(complete_trials))
    print("")
    print("Trials:")
    for trial in study.trials:
        if trial.state == TrialState.PRUNED:
            print("  Trial#{}: \tPRUNED, \tParams: {}".format(trial.number, trial.params))
        else:
            print("  Trial#{}: \tValue: {}, \tParams: {}, \tAtrr: {}".format(trial.number, trial.values, trial.params, trial.user_attrs))
    print("")
    print("Best trials:")
    sorted_best_trials = sorted(study.best_trials, key=lambda trial: (-trial.values[0], trial.values[1], trial.number))
    for trial in sorted_best_trials:
        print("  Trial#{}: \tValue: {}, \tParams: {}, \tAtrr: {}".format(trial.number, trial.values, trial.params, trial.user_attrs))
    print("")
    
    def trial_to_dict(t):
        return {
            'number': t.number,
            'state': str(t.state),
            'values': t.values,
            'params': t.params,
            'user_attrs': t.user_attrs
        }

    study_summary = {
        'n_finished_trials': len(study.trials),
        'n_pruned_trials': len(pruned_trials),
        'n_complete_trials': len(complete_trials),
        'trials': [trial_to_dict(t) for t in study.trials],
        'best_trials': [trial_to_dict(t) for t in sorted_best_trials]
    }

    summary_path = output_path('snn_study_summary.json')
    with open(summary_path, 'w', encoding='utf-8') as f:
        json.dump(study_summary, f, indent=2, ensure_ascii=False)
    
    fig = optuna.visualization.matplotlib.plot_pareto_front(study, target_names=["Dokładność", "Średnia liczba impulsów na próbkę"]).figure
    ax = fig.axes[0]
    ax.set_yscale("log", base=2)
    fig.savefig(output_path("snn_hyperparameter_optimization_pareto_front.pdf"))
    
    del train_loader._iterator # https://github.com/pytorch/pytorch/issues/64766
    del val_loader._iterator
