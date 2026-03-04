#pragma once
#include "cmd.hpp"

class CmdResetOdinCore : public Cmd
{
public:
    CmdResetOdinCore(OdinServer& server,
                     const FB::Cmd* fb_cmd,
                     const std::shared_ptr<std::vector<uint8_t>>& packet_data)
        : Cmd(server, fb_cmd, packet_data)
    {
    }

    static std::unique_ptr<CmdResetOdinCore> from_flatbuffer(OdinServer& server,
                                                             const FB::Cmd* fb_cmd,
                                                             const std::shared_ptr<std::vector<uint8_t>>& packet_data)
    {
        if (!fb_cmd || fb_cmd->t_type() != FB::Cmd_t_CmdResetOdinCore)
            return nullptr;
        return std::make_unique<CmdResetOdinCore>(server, fb_cmd, packet_data);
    }

    void execute() override
    {
        if (!fb_cmd)
            return;
        const auto* cmd = fb_cmd->t_as_CmdResetOdinCore();
        if (!cmd)
            return;

        uint8_t core_id = cmd->core_id();
        Logger::debug("Executing CmdResetOdinCore: core_id=%d", core_id);

        auto& cores = server.get_odin_cores();
        if (core_id >= cores.size()) {
            Logger::warning("CmdResetOdinCore: core_id %d out of range (max: %d)", core_id, (int)cores.size() - 1);
            return;
        }
        if (cores[core_id] == nullptr) {
            Logger::warning("CmdResetOdinCore: core_id %d is null", core_id);
            return;
        }

        cores[core_id]->reset();

        send_handle_response();
    }
};
