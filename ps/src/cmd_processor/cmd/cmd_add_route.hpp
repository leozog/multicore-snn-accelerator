#pragma once
#include "cmd.hpp"

extern "C"
{
#include "xil_types.h"
}

class CmdAddRoute : public Cmd
{
public:
    CmdAddRoute(OdinServer& server, const FB::Cmd* fb_cmd, const std::shared_ptr<std::vector<uint8_t>>& packet_data)
        : Cmd(server, fb_cmd, packet_data)
    {
    }

    static std::unique_ptr<CmdAddRoute> from_flatbuffer(OdinServer& server,
                                                        const FB::Cmd* fb_cmd,
                                                        const std::shared_ptr<std::vector<uint8_t>>& packet_data)
    {
        if (!fb_cmd || fb_cmd->t_type() != FB::Cmd_t_CmdAddRoute)
            return nullptr;
        return std::make_unique<CmdAddRoute>(server, fb_cmd, packet_data);
    }

    void execute() override
    {
        if (!fb_cmd)
            return;
        const auto* cmd = fb_cmd->t_as_CmdAddRoute();
        if (!cmd)
            return;

        u8 from_core = cmd->from_core();
        u8 from_neur = cmd->from_neur();
        u8 to_core = cmd->to_core();
        u32 event_addr = cmd->event_addr();
        Logger::debug("Executing CmdAddRoute: from_core=%d, from_neur=%d, to_core=%d, event_addr=%08X",
                      from_core,
                      from_neur,
                      to_core,
                      event_addr);

        auto& cores = server.get_odin_cores();
        if (from_core >= cores.size()) {
            Logger::warning("CmdAddRoute: from_core %d out of range (max: %d)", from_core, (int)cores.size() - 1);
            return;
        }
        if (cores[from_core] == nullptr) {
            Logger::warning("CmdAddRoute: from_core %d is null", from_core);
            return;
        }
        if (to_core >= cores.size()) {
            Logger::warning("CmdAddRoute: to_core %d out of range (max: %d)", to_core, (int)cores.size() - 1);
            return;
        }
        if (cores[to_core] == nullptr) {
            Logger::warning("CmdAddRoute: to_core %d is null", to_core);
            return;
        }
        if (from_neur >= 256) {
            Logger::warning("CmdAddRoute: from_neur %d out of range (max: 255)", from_neur);
            return;
        }

        server.get_event_router().routing_table.add_route(from_core, from_neur, to_core, event_addr);

        send_handle_response();
    }
};
