#pragma once
#include "cmd.hpp"

class CmdSetSynapse : public Cmd
{
public:
    CmdSetSynapse(OdinServer& server, const FB::Cmd* fb_cmd, const std::shared_ptr<std::vector<uint8_t>>& packet_data)
        : Cmd(server, fb_cmd, packet_data)
    {
    }

    static std::unique_ptr<CmdSetSynapse> from_flatbuffer(OdinServer& server,
                                                          const FB::Cmd* fb_cmd,
                                                          const std::shared_ptr<std::vector<uint8_t>>& packet_data)
    {
        if (!fb_cmd || fb_cmd->t_type() != FB::Cmd_t_CmdSetSynapse)
            return nullptr;
        return std::make_unique<CmdSetSynapse>(server, fb_cmd, packet_data);
    }

    void execute() override
    {
        if (!fb_cmd)
            return;
        const auto* cmd = fb_cmd->t_as_CmdSetSynapse();
        if (!cmd)
            return;

        uint8_t core_id = cmd->core_id();
        u32 pre_neur_id = cmd->pre_neur_id();
        u32 post_neur_id = cmd->post_neur_id();
        u8 weight = cmd->weight();
        Logger::debug("Executing CmdSetSynapse: core_id=%d, pre_neur_id=%u, post_neur_id=%u, weight=%u",
                      core_id,
                      pre_neur_id,
                      post_neur_id,
                      weight);

        auto& cores = server.get_odin_cores();
        if (core_id >= cores.size()) {
            Logger::warning("CmdSetSynapse: core_id %d out of range (max: %d)", core_id, (int)cores.size() - 1);
            return;
        }
        if (cores[core_id] == nullptr) {
            Logger::warning("CmdSetSynapse: core_id %d is null", core_id);
            return;
        }
        if (pre_neur_id >= 256) {
            Logger::warning("CmdSetSynapse: pre_neur_id %d out of range (max: 255)", pre_neur_id);
            return;
        }
        if (post_neur_id >= 256) {
            Logger::warning("CmdSetSynapse: post_neur_id %d out of range (max: 255)", post_neur_id);
            return;
        }

        cores[core_id]->set_synapse(pre_neur_id, post_neur_id, weight);

        send_handle_response();
    }
};
