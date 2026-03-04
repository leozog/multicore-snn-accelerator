#include "cmd/cmd_list.hpp"
#include "cmd_processor.hpp"
#include "flatbuffers/input_generated.h"
#include "logger.hpp"


std::unique_ptr<Cmd> CmdProcessor::cmd_from_flatbuffer(const FB::Cmd* fb_cmd,
                                                       const std::shared_ptr<std::vector<uint8_t>>& packet_data)
{
    if (!fb_cmd)
        return nullptr;

    switch (fb_cmd->t_type()) {
        case FB::Cmd_t_CmdResetAll:
            return CmdResetAll::from_flatbuffer(server, fb_cmd, packet_data);

        case FB::Cmd_t_CmdResetOdinCore:
            return CmdResetOdinCore::from_flatbuffer(server, fb_cmd, packet_data);

        case FB::Cmd_t_CmdWaitIdle:
            return CmdWaitIdle::from_flatbuffer(server, fb_cmd, packet_data);

        case FB::Cmd_t_CmdWaitOdinCore:
            return CmdWaitOdinCore::from_flatbuffer(server, fb_cmd, packet_data);

        case FB::Cmd_t_CmdSendPacket:
            return CmdSendPacket::from_flatbuffer(server, fb_cmd, packet_data);

        case FB::Cmd_t_CmdGetRegmem:
            return CmdGetRegmem::from_flatbuffer(server, fb_cmd, packet_data);

        case FB::Cmd_t_CmdSetRegmem:
            return CmdSetRegmem::from_flatbuffer(server, fb_cmd, packet_data);

        case FB::Cmd_t_CmdGetNeuron:
            return CmdGetNeuron::from_flatbuffer(server, fb_cmd, packet_data);

        case FB::Cmd_t_CmdSetNeuron:
            return CmdSetNeuron::from_flatbuffer(server, fb_cmd, packet_data);

        case FB::Cmd_t_CmdGetSynapse:
            return CmdGetSynapse::from_flatbuffer(server, fb_cmd, packet_data);

        case FB::Cmd_t_CmdSetSynapse:
            return CmdSetSynapse::from_flatbuffer(server, fb_cmd, packet_data);

        case FB::Cmd_t_CmdResetRouter:
            return CmdResetRouter::from_flatbuffer(server, fb_cmd, packet_data);

        case FB::Cmd_t_CmdAddRoute:
            return CmdAddRoute::from_flatbuffer(server, fb_cmd, packet_data);

        case FB::Cmd_t_CmdSetServerOutput:
            return CmdSetServerOutput::from_flatbuffer(server, fb_cmd, packet_data);

        default:
            Logger::warning("CmdDispatcher: Unknown command type %d", static_cast<int>(fb_cmd->t_type()));
            return nullptr;
    }
}
