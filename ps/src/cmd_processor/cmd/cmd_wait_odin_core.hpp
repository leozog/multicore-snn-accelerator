#pragma once
#include "cmd.hpp"

class CmdWaitOdinCore : public Cmd
{
public:
    CmdWaitOdinCore(OdinServer& server, const FB::Cmd* fb_cmd, const std::shared_ptr<std::vector<uint8_t>>& packet_data)
        : Cmd(server, fb_cmd, packet_data)
    {
    }

    static std::unique_ptr<CmdWaitOdinCore> from_flatbuffer(OdinServer& server,
                                                            const FB::Cmd* fb_cmd,
                                                            const std::shared_ptr<std::vector<uint8_t>>& packet_data)
    {
        if (!fb_cmd || fb_cmd->t_type() != FB::Cmd_t_CmdWaitOdinCore)
            return nullptr;
        return std::make_unique<CmdWaitOdinCore>(server, fb_cmd, packet_data);
    }

    void execute() override
    {
        if (!fb_cmd)
            return;
        const auto* cmd = fb_cmd->t_as_CmdWaitOdinCore();
        if (!cmd)
            return;

        uint8_t core_id = cmd->core_id();
        Logger::debug("Executing CmdWaitOdinCore: core_id=%d", core_id);

        auto& cores = server.get_odin_cores();
        if (core_id >= cores.size()) {
            Logger::warning("CmdWaitOdinCore: core_id %d out of range (max: %d)", core_id, (int)cores.size() - 1);
            return;
        }
        if (cores[core_id] == nullptr) {
            Logger::warning("CmdWaitOdinCore: core_id %d is null", core_id);
            return;
        }

        while (cores[core_id]->get_state() != ODIN<256, 8>::State::IDLE) {
            vTaskDelay(1);
        }

        send_handle_response();
    }
};
