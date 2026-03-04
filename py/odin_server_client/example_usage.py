from typing import Iterable
import threading
import argparse

from odin_server_client.odin_server_client import OdinServerClient
from odin_server_client.odin_neuron import OdinNeuron
from odin_server_client.odin_regmem import OdinRegmem
from odin_server_client.odin_synapse import OdinSynapse
from odin_server_client.helpers.mk_event import mk_neuron_spike_event
from odin_server_client.flatbuffers.FB import Cmd
from odin_server_client.flatbuffers.FB import CmdResetAll
from odin_server_client.flatbuffers.FB import CmdSendPacket
from odin_server_client.flatbuffers.FB import CmdSetServerOutput
from odin_server_client.flatbuffers.FB import CmdWaitOdinCore, CmdWaitIdle
from odin_server_client.flatbuffers.FB import Cmd_t

HOST = "169.254.5.1"
PORT = 5001

class SpikeCounter:
    def __init__(self):
        self.m = {}
        self.lock = threading.Lock()
    
    def __call__(self, core_id: int, spikes: Iterable[int]) -> None:
        with self.lock:
            for neur_id in spikes:
                key = (core_id, neur_id)
                self.m[key] = self.m.get(key, 0) + 1

    def count(self) -> None:
        with self.lock:
            print("count: ")
            for key, count in self.m.items():
                print(f"Core {key[0]} Neuron {key[1]} spiked {count} times")


def main():
    parser = argparse.ArgumentParser(description="Usage of OdinServerClient")
    parser.add_argument("--host", dest="host", default=HOST, help="Odin server host (default: %(default)s)")
    parser.add_argument("--port", dest="port", type=int, default=PORT, help="Odin server port (default: %(default)s)")
    args = parser.parse_args()

    spike_counter = SpikeCounter()

    with OdinServerClient(args.host, args.port, spikes_handler=spike_counter) as client:
        reset_cmd = Cmd.CmdT(tType=Cmd_t.Cmd_t.CmdResetAll, t=CmdResetAll.CmdResetAllT())
        client.send_command(reset_cmd, wait_for_output=False)

        set_reg_cmd = OdinRegmem.make_set_cmd(core_id=0, regmem={"open_loop": 1})
        client.send_command(set_reg_cmd, wait_for_output=False)
        out = client.send_command(OdinRegmem.make_get_cmd(core_id=0), wait_for_output=True)
        reg = OdinRegmem.decode_get_output(out)
        print("Regmem read:", reg)

        neuron0 = {"lif_izh_sel": 1, "thr": 5, "leak_str": 0, "leak_en": 0, "core": 0, "neur_disable": 0}
        neuron1 = {"lif_izh_sel": 1, "thr": 4, "leak_str": 0, "leak_en": 0, "core": 0, "neur_disable": 0}
        client.send_command(OdinNeuron.make_set_cmd(core_id=0, neur_id=5, neuron=neuron0), wait_for_output=False)
        client.send_command(OdinNeuron.make_set_cmd(core_id=0, neur_id=6, neuron=neuron1), wait_for_output=False)
        out5 = client.send_command(OdinNeuron.make_get_cmd(core_id=0, neur_id=5), wait_for_output=True)
        out6 = client.send_command(OdinNeuron.make_get_cmd(core_id=0, neur_id=6), wait_for_output=True)
        n5 = OdinNeuron.decode_get_output(out5)
        n6 = OdinNeuron.decode_get_output(out6)
        print("Neuron5 read:", n5)
        print("Neuron6 read:", n6)
        if n5["neuron"]["thr"] != 5:
            print("Config readback mismatch: neuron5 thr", n5["neuron"]["thr"])
        if n6["neuron"]["thr"] != 4:
            print("Config readback mismatch: neuron6 thr", n6["neuron"]["thr"])

        client.send_command(OdinSynapse.make_set_cmd(core_id=0, pre_neur_id=10, post_neur_id=5, weight=3, mapping_bit=1), wait_for_output=False)
        client.send_command(OdinSynapse.make_set_cmd(core_id=0, pre_neur_id=11, post_neur_id=5, weight=3, mapping_bit=1), wait_for_output=False)
        client.send_command(OdinSynapse.make_set_cmd(core_id=0, pre_neur_id=10, post_neur_id=6, weight=4, mapping_bit=1), wait_for_output=False)
        s10_5 = OdinSynapse.decode_get_output(client.send_command(OdinSynapse.make_get_cmd(core_id=0, pre_neur_id=10, post_neur_id=5), wait_for_output=True))
        s11_5 = OdinSynapse.decode_get_output(client.send_command(OdinSynapse.make_get_cmd(core_id=0, pre_neur_id=11, post_neur_id=5), wait_for_output=True))
        s10_6 = OdinSynapse.decode_get_output(client.send_command(OdinSynapse.make_get_cmd(core_id=0, pre_neur_id=10, post_neur_id=6), wait_for_output=True))
        print("Synapse 10->5:", s10_5)
        print("Synapse 11->5:", s11_5)
        print("Synapse 10->6:", s10_6)
        if s10_5["weight"] != 3 or s10_5["mapping_bit"] != 1:
            print("Config readback mismatch: syn10_5", s10_5)
        if s11_5["weight"] != 3 or s11_5["mapping_bit"] != 1:
            print("Config readback mismatch: syn11_5", s11_5)
        if s10_6["weight"] != 4 or s10_6["mapping_bit"] != 1:
            print("Config readback mismatch: syn10_6", s10_6)

        # send all the commands all at once from this point 
        commands = []
        commands.append(Cmd.CmdT(
            tType=Cmd_t.Cmd_t.CmdSetServerOutput,
            t=CmdSetServerOutput.CmdSetServerOutputT(0, 5, True)
        ))
        commands.append(Cmd.CmdT(
            tType=Cmd_t.Cmd_t.CmdSetServerOutput,
            t=CmdSetServerOutput.CmdSetServerOutputT(0, 6, True)
        ))
        
        words = []
        for i in range(10000):
            words.append(mk_neuron_spike_event(10))
            words.append(mk_neuron_spike_event(11))
        commands.append(Cmd.CmdT(
            tType=Cmd_t.Cmd_t.CmdSendPacket,
            t=CmdSendPacket.CmdSendPacketT(coreId=0, words=words),
        ))
        commands.append(Cmd.CmdT(
            tType=Cmd_t.Cmd_t.CmdSendPacket,
            t=CmdSendPacket.CmdSendPacketT(coreId=0, words=words),
        ))
        commands.append(Cmd.CmdT(
            tType=Cmd_t.Cmd_t.CmdWaitIdle,
            t=CmdWaitIdle.CmdWaitIdleT()
        ))
        client.send_commands(commands, wait_for_outputs=True)
        
        print("Open-loop LIF test end")
        
        # import time 
        # time.sleep(.01) # wait for all output spike packets to arive
        
        spike_counter.count()


if __name__ == "__main__":
    main()
