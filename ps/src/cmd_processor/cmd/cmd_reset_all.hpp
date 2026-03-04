#pragma once
#include "cmd.hpp"

class CmdResetAll : public Cmd
{
public:
    CmdResetAll(OdinServer& server, const FB::Cmd* fb_cmd, const std::shared_ptr<std::vector<uint8_t>>& packet_data)
        : Cmd(server, fb_cmd, packet_data)
    {
    }

    static std::unique_ptr<CmdResetAll> from_flatbuffer(OdinServer& server,
                                                        const FB::Cmd* fb_cmd,
                                                        const std::shared_ptr<std::vector<uint8_t>>& packet_data)
    {
        if (!fb_cmd || fb_cmd->t_type() != FB::Cmd_t_CmdResetAll)
            return nullptr;
        return std::make_unique<CmdResetAll>(server, fb_cmd, packet_data);
    }

    ExecutionMode get_execution_mode() const override { return ExecutionMode::IMMEDIATE; }

    void execute() override
    {
        if (!fb_cmd)
            return;
        Logger::debug("Executing CmdResetAll");

        auto& event_router = server.get_event_router();
        event_router.reset();

        auto& odin_cores = server.get_odin_cores();
        for (auto& core : odin_cores) {
            if (core != nullptr) {
                core->reset();
            }
        }

        event_router.reset();

        auto& cmd_processor = server.get_cmd_processor();
        cmd_processor.clear_pending_commands();

        server.get_cmd_processor().clear_pending_commands();

        send_handle_response();
    }
};
