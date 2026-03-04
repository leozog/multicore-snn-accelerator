from typing import Dict, Iterable, List, Mapping, Tuple

from .flatbuffers.FB import Cmd as FB_Cmd
from .flatbuffers.FB import CmdGetNeuron as FB_CmdGetNeuron
from .flatbuffers.FB import CmdOutput_t as FB_CmdOutput_t
from .flatbuffers.FB import CmdSetNeuron as FB_CmdSetNeuron
from .flatbuffers.FB import Cmd_t as FB_Cmd_t
from .flatbuffers.FB import NeuronWords128 as FB_NeuronWords128


class OdinNeuron:
    LIF = (
        ("lif_izh_sel", 0, 1), ("leak_str", 1, 7), ("leak_en", 8, 1), ("thr", 9, 8), ("ca_en", 17, 1),
        ("thetamem", 18, 8), ("ca_theta1", 26, 3), ("ca_theta2", 29, 3), ("ca_theta3", 32, 3),
        ("ca_leak", 35, 5), ("lif_reserved", 40, 30), ("core", 70, 8), ("calcium", 78, 3),
        ("caleak_cnt", 81, 5), ("lif_reserved2", 86, 41), ("neur_disable", 127, 1),
    )
    IZH = (
        ("lif_izh_sel", 0, 1), ("leak_str", 1, 7), ("leak_en", 8, 1), ("fi_sel", 9, 3),
        ("spk_ref", 12, 3), ("isi_ref", 15, 3), ("reson_sharp_en", 18, 1), ("thr", 19, 3),
        ("rfr", 22, 3), ("dapdel", 25, 3), ("spklat_en", 28, 1), ("dap_en", 29, 1),
        ("stim_thr", 30, 3), ("phasic_en", 33, 1), ("mixed_en", 34, 1), ("class2_en", 35, 1),
        ("neg_en", 36, 1), ("rebound_en", 37, 1), ("inhin_en", 38, 1), ("bist_en", 39, 1),
        ("reson_en", 40, 1), ("thrvar_en", 41, 1), ("thr_sel_of", 42, 1), ("thrleak", 43, 4),
        ("acc_en", 47, 1), ("ca_en", 48, 1), ("thetamem", 49, 3), ("ca_theta1", 52, 3),
        ("ca_theta2", 55, 3), ("ca_theta3", 58, 3), ("ca_leak", 61, 5), ("burst_incr", 66, 1),
        ("reson_sharp_amt", 67, 3), ("inacc", 70, 11), ("refrac", 81, 1), ("core", 82, 4),
        ("dapdel_cnt", 86, 3), ("stim_str", 89, 4), ("stim_str_tmp", 93, 4), ("phasic_lock", 97, 1),
        ("mixed_lock", 98, 1), ("spkout_done", 99, 1), ("stim0_prev", 100, 2), ("inhexc_prev", 102, 2),
        ("bist_lock", 104, 1), ("inhin_lock", 105, 1), ("reson_sign", 106, 2), ("thrmod", 108, 4),
        ("thrleak_cnt", 112, 4), ("calcium", 116, 3), ("caleak_cnt", 119, 5), ("burst_lock", 124, 1),
        ("izh_reserved", 125, 2), ("neur_disable", 127, 1),
    )

    @staticmethod
    def pack(n: Mapping[str, int]) -> List[int]:
        if "lif_izh_sel" not in n:
            raise ValueError("Field 'lif_izh_sel' is required")
        sel = n["lif_izh_sel"]
        if type(sel) is not int or sel not in (0, 1):
            raise ValueError("lif_izh_sel must be 0 or 1")
        specs = OdinNeuron.LIF if sel == 1 else OdinNeuron.IZH
        raw = 0
        for name, start, width in specs:
            v = n.get(name, 0)
            if type(v) is not int or v < 0 or v >= (1 << width):
                raise ValueError(f"{name} out of range")
            raw |= (v & ((1 << width) - 1)) << start
        return [(raw >> (i * 32)) & 0xFFFFFFFF for i in range(4)]

    @staticmethod
    def unpack(words: Iterable[int]) -> Dict[str, int]:
        w = list(words)
        if len(w) != 4:
            raise ValueError("Need 4 words")
        raw = sum(int(w[i]) << (32 * i) for i in range(4))
        specs = OdinNeuron.LIF if raw & 1 else OdinNeuron.IZH
        return {n: (raw >> s) & ((1 << w) - 1) for n, s, w in specs}

    @staticmethod
    def make_set_cmd(core_id: int, neur_id: int, neuron: Mapping[str, int]):
        return FB_Cmd.CmdT(
            tType=FB_Cmd_t.Cmd_t.CmdSetNeuron,
            t=FB_CmdSetNeuron.CmdSetNeuronT(
                coreId=core_id, neurId=neur_id, neuronWords=FB_NeuronWords128.NeuronWords128T(words=OdinNeuron.pack(neuron))
            ),
        )

    @staticmethod
    def make_get_cmd(core_id: int, neur_id: int):
        return FB_Cmd.CmdT(tType=FB_Cmd_t.Cmd_t.CmdGetNeuron, t=FB_CmdGetNeuron.CmdGetNeuronT(coreId=core_id, neurId=neur_id))

    @staticmethod
    def decode_get_output(out):
        if out.tType != FB_CmdOutput_t.CmdOutput_t.CmdOutputGetNeuron:
            raise ValueError("Expected CmdOutputGetNeuron")
        p = out.t
        w = [int(v) for v in p.neuronWords.words]
        return {"core_id": int(p.coreId), "neur_id": int(p.neurId), "words": w, "neuron": OdinNeuron.unpack(w)}