from typing import Dict, Iterable, List, Mapping

from .flatbuffers.FB import Cmd as FB_Cmd
from .flatbuffers.FB import CmdGetRegmem as FB_CmdGetRegmem
from .flatbuffers.FB import CmdOutput_t as FB_CmdOutput_t
from .flatbuffers.FB import CmdSetRegmem as FB_CmdSetRegmem
from .flatbuffers.FB import Cmd_t as FB_Cmd_t
from .flatbuffers.FB import OdinRegmem as FB_OdinRegmem


class OdinRegmem:
    @staticmethod
    def pack(r: Mapping[str, object]) -> FB_OdinRegmem.OdinRegmemT:
        def u32(k):
            v = r.get(k, 0)
            if type(v) is not int or v < 0 or v > 0xFFFFFFFF:
                raise ValueError(f"{k} must be u32")
            return v

        def u32_list(k):
            v = r.get(k)
            if v is None:
                return [0] * 8
            if not isinstance(v, list) or len(v) != 8:
                raise ValueError(f"{k} must be list[8]")
            for i in v:
                if type(i) is not int or i < 0 or i > 0xFFFFFFFF:
                    raise ValueError(f"{k} must be u32 list")
            return v

        return FB_OdinRegmem.OdinRegmemT(
            openLoop=u32("open_loop"), synSign=u32_list("syn_sign"), reservedSynSign=u32_list("reserved_syn_sign"),
            burstTimeref=u32("burst_timeref"), aerSrcCtrlNneur=u32("aer_src_ctrl_nneur"),
            outAerMonitorEn=u32("out_aer_monitor_en"), monitorNeurAddr=u32("monitor_neur_addr"),
            monitorSynAddr=u32("monitor_syn_addr"), updateUnmappedSyn=u32("update_unmapped_syn"),
            propagateUnmappedSyn=u32("propagate_unmapped_syn"), sdspOnSynStim=u32("sdsp_on_syn_stim"),
        )

    @staticmethod
    def unpack(r: FB_OdinRegmem.OdinRegmemT) -> Dict[str, object]:
        return {
            "open_loop": int(r.openLoop), "syn_sign": [int(v) for v in (r.synSign if r.synSign is not None else [0] * 8)],
            "reserved_syn_sign": [int(v) for v in (r.reservedSynSign if r.reservedSynSign is not None else [0] * 8)],
            "burst_timeref": int(r.burstTimeref), "aer_src_ctrl_nneur": int(r.aerSrcCtrlNneur),
            "out_aer_monitor_en": int(r.outAerMonitorEn), "monitor_neur_addr": int(r.monitorNeurAddr),
            "monitor_syn_addr": int(r.monitorSynAddr), "update_unmapped_syn": int(r.updateUnmappedSyn),
            "propagate_unmapped_syn": int(r.propagateUnmappedSyn), "sdsp_on_syn_stim": int(r.sdspOnSynStim),
        }

    @staticmethod
    def make_set_cmd(core_id: int, regmem: Mapping[str, object]):
        return FB_Cmd.CmdT(tType=FB_Cmd_t.Cmd_t.CmdSetRegmem, t=FB_CmdSetRegmem.CmdSetRegmemT(coreId=core_id, regmem=OdinRegmem.pack(regmem)))

    @staticmethod
    def make_get_cmd(core_id: int):
        return FB_Cmd.CmdT(tType=FB_Cmd_t.Cmd_t.CmdGetRegmem, t=FB_CmdGetRegmem.CmdGetRegmemT(coreId=core_id))

    @staticmethod
    def decode_get_output(out):
        if out.tType != FB_CmdOutput_t.CmdOutput_t.CmdOutputGetRegmem:
            raise ValueError("Expected CmdOutputGetRegmem")
        return {"core_id": int(out.t.coreId), "regmem": OdinRegmem.unpack(out.t.regmem)}
