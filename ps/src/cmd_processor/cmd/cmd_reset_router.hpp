#pragma once
#include "cmd.hpp"

extern "C"
{
#include "xil_types.h"
}

class CmdResetRouter : public Cmd
{
public:
    CmdResetRouter(OdinServer& server, const FB::Cmd* fb_cmd, const std::shared_ptr<std::vector<uint8_t>>& packet_data)
        : Cmd(server, fb_cmd, packet_data)
    {
    }

    static std::unique_ptr<CmdResetRouter> from_flatbuffer(OdinServer& server,
                                                           const FB::Cmd* fb_cmd,
                                                           const std::shared_ptr<std::vector<uint8_t>>& packet_data)
    {
        if (!fb_cmd || fb_cmd->t_type() != FB::Cmd_t_CmdResetRouter)
            return nullptr;
        return std::make_unique<CmdResetRouter>(server, fb_cmd, packet_data);
    }

    void execute() override
    {
        if (!fb_cmd)
            return;
        const auto* cmd = fb_cmd->t_as_CmdResetRouter();
        if (!cmd)
            return;

        Logger::debug("Executing CmdResetRouter");

        auto& event_router = server.get_event_router();
        event_router.reset();

        send_handle_response();
    }
};
