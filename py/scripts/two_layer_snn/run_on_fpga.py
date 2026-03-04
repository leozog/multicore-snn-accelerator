import argparse
import json
import threading
import time
from typing import Any, Dict, List

import torch
import numpy as np
from spikingjelly.activation_based import encoding

from common.get_mnist_loaders import get_mnist_loaders
from common.load_json import load_json
from common.transform.minst_transform import ResizeDeskewThresholdTransform, ResizeDeskewTransform
from common.nn_model.eval_model import eval_model

from odin_server_client.odin_regmem import OdinRegmem
from odin_server_client.odin_server_client import OdinServerClient
from odin_server_client.helpers.program_memory import program_synram, program_lif_neurons
from odin_server_client.helpers.mk_event import mk_neuron_spike_event, mk_all_neurons_time_ref
from odin_server_client.flatbuffers.FB import Cmd, CmdWaitOdinCore
from odin_server_client.flatbuffers.FB import CmdResetAll, CmdSendPacket, CmdSetServerOutput, CmdWaitIdle, CmdAddRoute
from odin_server_client.flatbuffers.FB import Cmd_t


HOST = "169.254.5.1"
PORT = 5001
AVAILABLE_CORES = 5


class ODIN_TWO_LAYER_SNN():
    def __init__(self, client: OdinServerClient, json_data: Dict[str, Any]):
        self.client = client
        
        self.sections = json_data['sections']
        self.fusion = json_data['fusion']
        self.timesteps = json_data.get('timesteps', 1)
        self.encoder = encoding.PoissonEncoder(step_mode='m')
        
        self.num_sections = len(self.sections)
        self.fusion_core_id = self.num_sections
        
        client.spikes_handler = self.handle_spikes
        self.lock = threading.Lock()
        self.total_spikes = 0
        self.total_call_time = 0.0

        commands = []
        commands.append(Cmd.CmdT(
            tType=Cmd_t.Cmd_t.CmdResetAll,
            t=CmdResetAll.CmdResetAllT()
        ))
        
        for s_idx, sec in enumerate(self.sections):
            commands.append(OdinRegmem.make_set_cmd(core_id=s_idx, regmem={"open_loop": 1}))
            weight = np.array(sec['linear_weight'], dtype=np.int8)
            commands.extend(program_synram(self.client, core_id=s_idx, weights=weight))
        
        section_out = len(self.sections[0]['v_thr_q'])
        for s_idx in range(self.num_sections):
            from_core = s_idx
            to_core = self.fusion_core_id
            for neur_j in range(section_out):
                target_pre = s_idx * section_out + neur_j
                commands.append(Cmd.CmdT(
                    tType=Cmd_t.Cmd_t.CmdAddRoute,
                    t=CmdAddRoute.CmdAddRouteT(from_core, neur_j, to_core, mk_neuron_spike_event(target_pre))
                ))
        
        commands.append(OdinRegmem.make_set_cmd(core_id=self.fusion_core_id , regmem={"open_loop": 1}))
        fusion_weight = np.array(self.fusion['linear_weight'], dtype=np.int8)
        commands.extend(program_synram(self.client, core_id=self.fusion_core_id, weights=fusion_weight))
        out_features = len(self.fusion['v_thr_q'])
        for neur_id in range(out_features):
            commands.append(Cmd.CmdT(
                tType=Cmd_t.Cmd_t.CmdSetServerOutput,
                t=CmdSetServerOutput.CmdSetServerOutputT(self.fusion_core_id, neur_id, True)
            ))
        self.client.send_commands(commands, wait_for_outputs=False)
    
    def eval(self):
        return 0

    def handle_spikes(self, core_id: int, neur_ids):
        with self.lock:
            for neur_id in neur_ids:
                key = (core_id, neur_id)
                self.spike_counts[key] = self.spike_counts.get(key, 0) + 1

    def reset(self):
        with self.lock:
            self.spike_counts = {}
        commands = []
        
        for s_idx, sec in enumerate(self.sections):
            v_thr = np.array(sec['v_thr_q'], dtype=np.int8)
            leak = np.array(sec['leak_str_q'], dtype=np.int8)
            commands.extend(program_lif_neurons(self.client, core_id=s_idx, v_thr=v_thr, leak_str=leak))
        
        v_thr = np.array(self.fusion['v_thr_q'], dtype=np.int8)
        leak = np.array(self.fusion['leak_str_q'], dtype=np.int8)
        commands.extend(program_lif_neurons(self.client, core_id=self.fusion_core_id, v_thr=v_thr, leak_str=leak))
        return commands

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

    def __call__(self, x: torch.Tensor):
        commands = []
        commands.extend(self.reset())

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

        start = time.perf_counter()
        in_spikes = 0
        
        for t in range(self.timesteps):
            commands.append(Cmd.CmdT(
                tType=Cmd_t.Cmd_t.CmdWaitOdinCore,
                t=CmdWaitOdinCore.CmdWaitOdinCoreT(coreId=self.fusion_core_id)
            ))
            
            words = []
            words.append(mk_all_neurons_time_ref())
            commands.append(Cmd.CmdT(
                tType=Cmd_t.Cmd_t.CmdSendPacket,
                t=CmdSendPacket.CmdSendPacketT(coreId=self.fusion_core_id, words=words),
            ))
            for s_idx in range(self.num_sections):
                words = []
                
                words.append(mk_all_neurons_time_ref())
                
                arr = section_inputs[s_idx].detach().cpu().numpy()
                pre_spikes = (arr[t, 0] > 0).nonzero()[0]
                if pre_spikes.size > 0:
                    words.extend([mk_neuron_spike_event(int(p)) for p in pre_spikes])
                commands.append(Cmd.CmdT(
                    tType=Cmd_t.Cmd_t.CmdSendPacket,
                    t=CmdSendPacket.CmdSendPacketT(coreId=s_idx, words=words),
                ))
                in_spikes += len(words)
                
                self.client.send_commands(commands, wait_for_outputs=False)
                commands = []

        print("in_spikes:", in_spikes)
        self.total_spikes += in_spikes
        
        commands.append(Cmd.CmdT(
            tType=Cmd_t.Cmd_t.CmdWaitIdle,
            t=CmdWaitIdle.CmdWaitIdleT()
        ))
        self.client.send_commands(commands, wait_for_outputs=True)

        duration = time.perf_counter() - start
        self.total_call_time += duration
        
        out_features = len(self.fusion['v_thr_q'])
        counts = torch.zeros((1, out_features), dtype=torch.float32, device=x.device)
        with self.lock:
            for (core_id, neur_id), cnt in self.spike_counts.items():
                if core_id == self.fusion_core_id: # there shouldnt be any spikes from other cores, but check just in case
                    counts[0, neur_id] = float(cnt)

        return counts

    def avg_time_per_spike(self) -> float:
        return (self.total_call_time / self.total_spikes) if self.total_spikes > 0 else 0.0


def main():
    parser = argparse.ArgumentParser(description='Run two-layer SNN on ODIN FPGA')
    parser.add_argument("--host", dest="host", default=HOST)
    parser.add_argument("--port", dest="port", type=int, default=PORT)
    parser.add_argument('--json_path', help='Snn trial JSON file', required=True)
    args = parser.parse_args()

    json_data = load_json(args.json_path)

    transform = ResizeDeskewThresholdTransform(resize=(28, 28), threshold=json_data.get('soft_threshold', 0.0))
    _, val_loader = get_mnist_loaders(batch_size=1, transform=transform, fraction=0.02)

    criterion = torch.nn.CrossEntropyLoss()

    print("Starting evaluation")
    with OdinServerClient(args.host, args.port) as client:
        snn = ODIN_TWO_LAYER_SNN(client=client, json_data=json_data)
        eval_loss, eval_acc = eval_model(device='cpu', model=snn, criterion=criterion, loader=val_loader)
        print(f"eval_loss: {eval_loss}, eval_acc: {eval_acc}")

        avg_spike_time = snn.avg_time_per_spike()
        print(f"avg_time_per_spike: {avg_spike_time:.9f}s")


if __name__ == '__main__':
    main()
