#pragma once
#include "cmd.hpp"
#include "event_router/event_router.hpp"

class CmdWaitIdle : public Cmd
{
public:
    CmdWaitIdle(OdinServer& server, const FB::Cmd* fb_cmd, const std::shared_ptr<std::vector<uint8_t>>& packet_data)
        : Cmd(server, fb_cmd, packet_data)
    {
    }

    static std::unique_ptr<CmdWaitIdle> from_flatbuffer(OdinServer& server,
                                                        const FB::Cmd* fb_cmd,
                                                        const std::shared_ptr<std::vector<uint8_t>>& packet_data)
    {
        if (!fb_cmd || fb_cmd->t_type() != FB::Cmd_t_CmdWaitIdle)
            return nullptr;
        return std::make_unique<CmdWaitIdle>(server, fb_cmd, packet_data);
    }

    void execute() override
    {
        if (!fb_cmd)
            return;
        const auto* cmd = fb_cmd->t_as_CmdWaitIdle();
        if (!cmd)
            return;

        Logger::debug("Executing CmdWaitIdle");

        bool all_idle = false;
        while (!all_idle) {
            all_idle = true;
            for (auto& core : server.get_odin_cores()) {
                if (core != nullptr) {
                    while (core->get_state() != ODIN<256, 8>::State::IDLE) {
                        all_idle = false;
                    }
                }
            }
            if (server.get_event_router().get_state() != EventRouter::State::IDLE) {
                all_idle = false;
            }
            vTaskDelay(1);
        }

        send_handle_response();
    }
};
