from typing import Dict, Mapping

from .flatbuffers.FB import Cmd as FB_Cmd
from .flatbuffers.FB import CmdGetSynapse as FB_CmdGetSynapse
from .flatbuffers.FB import CmdOutput_t as FB_CmdOutput_t
from .flatbuffers.FB import CmdSetSynapse as FB_CmdSetSynapse
from .flatbuffers.FB import Cmd_t as FB_Cmd_t


class OdinSynapse:
    @staticmethod
    def make_set_cmd(core_id: int, pre_neur_id: int, post_neur_id: int, weight: int, mapping_bit: int = 0):
        return FB_Cmd.CmdT(
            tType=FB_Cmd_t.Cmd_t.CmdSetSynapse,
            t=FB_CmdSetSynapse.CmdSetSynapseT(
                coreId=core_id, preNeurId=pre_neur_id, postNeurId=post_neur_id, weight=(weight & 7) | ((mapping_bit & 1) << 3)
            ),
        )

    @staticmethod
    def make_get_cmd(core_id: int, pre_neur_id: int, post_neur_id: int):
        return FB_Cmd.CmdT(
            tType=FB_Cmd_t.Cmd_t.CmdGetSynapse,
            t=FB_CmdGetSynapse.CmdGetSynapseT(coreId=core_id, preNeurId=pre_neur_id, postNeurId=post_neur_id),
        )

    @staticmethod
    def decode_get_output(out) -> Dict[str, int]:
        if out.tType != FB_CmdOutput_t.CmdOutput_t.CmdOutputGetSynapse:
            raise ValueError("Expected CmdOutputGetSynapse")
        p = out.t
        w = int(p.weight)
        return {"core_id": int(p.coreId), "pre_neur_id": int(p.preNeurId), "post_neur_id": int(p.postNeurId), "weight": w & 7, "mapping_bit": (w >> 3) & 1}
