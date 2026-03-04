import argparse
import json
import os
import threading
from typing import Any, Dict

import torch
import numpy as np
import time
from common.get_mnist_loaders import get_mnist_loaders
from common.load_json import load_json
import torch.nn as nn
from spikingjelly.activation_based import encoding
from common.nn_model.eval_model import eval_model

from common.transform.minst_transform import ResizeDeskewThresholdTransform
from odin_server_client.flatbuffers.FB.Cmd import Cmd
from odin_server_client.odin_regmem import OdinRegmem
from odin_server_client.odin_server_client import OdinServerClient
from odin_server_client.odin_synapse import OdinSynapse
from odin_server_client.odin_neuron import OdinNeuron
from odin_server_client.helpers.program_memory import program_synram, program_lif_neurons
from odin_server_client.helpers.mk_event import mk_neuron_spike_event, mk_all_neurons_time_ref
from odin_server_client.flatbuffers.FB import Cmd
from odin_server_client.flatbuffers.FB import CmdResetAll
from odin_server_client.flatbuffers.FB import CmdSendPacket
from odin_server_client.flatbuffers.FB import CmdSetServerOutput
from odin_server_client.flatbuffers.FB import CmdWaitOdinCore, CmdWaitIdle
from odin_server_client.flatbuffers.FB import Cmd_t


HOST = "169.254.5.1"
PORT = 5001

class ODIN_ONE_LAYER_SNN():
    def __init__(self, client: OdinServerClient, core_count: int, json_data):
        self.client = client
        self.core_count = core_count
        self.timesteps = json_data.get('timesteps', 1)
        self.linear_weight = np.array(json_data.get('linear_weight'), dtype=np.int8)
        self.v_thr_q = np.array(json_data.get('v_thr_q'), dtype=np.int8)
        self.leak_str_q = np.array(json_data.get('leak_str_q'), dtype=np.int8)
        self.encoder = encoding.PoissonEncoder(step_mode='m')
        self.out_features = len(self.v_thr_q)
        
        client.spikes_handler = self.handle_spikes
        
        self.total_spikes = 0
        self.total_call_time = 0.0
        
        commands = []
        commands.append(Cmd.CmdT(
                    tType=Cmd_t.Cmd_t.CmdResetAll,
                    t=CmdResetAll.CmdResetAllT()
                ))
        for core_id in range(self.core_count):
            
            commands.append(OdinRegmem.make_set_cmd(core_id=core_id, regmem={"open_loop": 1}))
            
            commands.extend(program_synram(self.client, core_id=core_id, weights=self.linear_weight))
            
            for neur_id in range(self.out_features):
                commands.append(Cmd.CmdT(
                    tType=Cmd_t.Cmd_t.CmdSetServerOutput,
                    t=CmdSetServerOutput.CmdSetServerOutputT(core_id, neur_id, True)
                ))
        self.client.send_commands(commands, wait_for_outputs=False)

        self.lock = threading.Lock()

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
        for core_id in range(self.core_count):
            commands.extend(program_lif_neurons(self.client, core_id=core_id, v_thr=self.v_thr_q, leak_str=self.leak_str_q))
        return commands

    def __call__(self, x):
        commands = []
        commands.extend(self.reset())
        
        x_flat = x.flatten(1)  # (batch, in_features)
        
        x_seq = x_flat.unsqueeze(0).repeat(self.timesteps, 1, 1)
        
        x_seq = self.encoder(x_seq)

        start = time.perf_counter()
        
        in_spikes = 0
        
        x_np = x_seq.detach().cpu().numpy()
        for t in range(self.timesteps):
            for core_id in range(self.core_count):
                words = []
                
                words.append(mk_all_neurons_time_ref())
                
                pre_spikes = (x_np[t, core_id] > 0).nonzero()[0]
                if pre_spikes.size > 0:
                    words.extend([mk_neuron_spike_event(int(p)) for p in pre_spikes])
                commands.append(Cmd.CmdT(
                    tType=Cmd_t.Cmd_t.CmdSendPacket,
                    t=CmdSendPacket.CmdSendPacketT(coreId=core_id, words=words),
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
        
        counts = torch.zeros((self.core_count, self.out_features), dtype=torch.float32, device=x.device)
        with self.lock:
            for (core_id, neur_id), cnt in self.spike_counts.items():
                counts[core_id, neur_id] = float(cnt)

        return counts

    def avg_time_per_spike(self) -> float:
        return (self.total_call_time / self.total_spikes) if self.total_spikes > 0 else 0.0


def main():
    parser = argparse.ArgumentParser(description='Load trial JSON and run it on OdinServer')
    parser.add_argument("--host", dest="host", default=HOST, help="Odin server host (default: %(default)s)")
    parser.add_argument("--port", dest="port", type=int, default=PORT, help="Odin server port (default: %(default)s)")
    parser.add_argument('--json_path', help='Snn trial JSON file', required=True)
    parser.add_argument('--core_count', help='Number of cores to use', type=int, default=1)
    args = parser.parse_args()

    print("Loading JSON data")
    json_data = load_json(args.json_path)

    transform = ResizeDeskewThresholdTransform(resize=(16, 16), threshold=json_data.get('soft_threshold', 0.0))
    _, val_loader = get_mnist_loaders(batch_size=args.core_count, transform=transform, fraction=0.05)

    criterion = nn.CrossEntropyLoss()

    print("Starting evaluation")
    with OdinServerClient(args.host, args.port) as client:
        snn = ODIN_ONE_LAYER_SNN(client=client, core_count=args.core_count, json_data=json_data)
        eval_loss, eval_acc = eval_model(device='cpu', model=snn, criterion=criterion, loader=val_loader)
        print(f"eval_loss: {eval_loss}, eval_acc: {eval_acc}")

        avg_spike_time = snn.avg_time_per_spike()
        print(f"avg_time_per_spike: {avg_spike_time:.9f}s")

if __name__ == '__main__':
    main()
