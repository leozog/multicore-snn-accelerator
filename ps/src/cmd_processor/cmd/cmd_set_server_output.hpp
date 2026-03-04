#pragma once
#include "cmd.hpp"

extern "C"
{
#include "xil_types.h"
}

class CmdSetServerOutput : public Cmd
{
public:
    CmdSetServerOutput(OdinServer& server,
                       const FB::Cmd* fb_cmd,
                       const std::shared_ptr<std::vector<uint8_t>>& packet_data)
        : Cmd(server, fb_cmd, packet_data)
    {
    }

    static std::unique_ptr<CmdSetServerOutput> from_flatbuffer(OdinServer& server,
                                                               const FB::Cmd* fb_cmd,
                                                               const std::shared_ptr<std::vector<uint8_t>>& packet_data)
    {
        if (!fb_cmd || fb_cmd->t_type() != FB::Cmd_t_CmdSetServerOutput)
            return nullptr;
        return std::make_unique<CmdSetServerOutput>(server, fb_cmd, packet_data);
    }

    void execute() override
    {
        if (!fb_cmd)
            return;
        const auto* cmd = fb_cmd->t_as_CmdSetServerOutput();
        if (!cmd)
            return;

        u8 from_core = cmd->from_core();
        u8 from_neur = cmd->from_neur();
        bool server_output = cmd->server_output();
        Logger::debug("Executing CmdSetServerOutput: from_core=%d, from_neur=%d, server_output=%s",
                      from_core,
                      from_neur,
                      server_output ? "true" : "false");

        auto& cores = server.get_odin_cores();
        if (from_core >= cores.size()) {
            Logger::warning(
                "CmdSetServerOutput: from_core %d out of range (max: %d)", from_core, (int)cores.size() - 1);
            return;
        }
        if (cores[from_core] == nullptr) {
            Logger::warning("CmdSetServerOutput: from_core %d is null", from_core);
            return;
        }
        if (from_neur >= 256) {
            Logger::warning("CmdSetServerOutput: from_neur %d out of range (max: 255)", from_neur);
            return;
        }

        server.get_event_router().routing_table.set_server_output(from_core, from_neur, server_output);

        send_handle_response();
    }
};
