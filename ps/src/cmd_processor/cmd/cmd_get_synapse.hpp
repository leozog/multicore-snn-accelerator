#pragma once
#include "cmd.hpp"

class CmdGetSynapse : public Cmd
{
public:
    CmdGetSynapse(OdinServer& server, const FB::Cmd* fb_cmd, const std::shared_ptr<std::vector<uint8_t>>& packet_data)
        : Cmd(server, fb_cmd, packet_data)
    {
    }

    static std::unique_ptr<CmdGetSynapse> from_flatbuffer(OdinServer& server,
                                                          const FB::Cmd* fb_cmd,
                                                          const std::shared_ptr<std::vector<uint8_t>>& packet_data)
    {
        if (!fb_cmd || fb_cmd->t_type() != FB::Cmd_t_CmdGetSynapse)
            return nullptr;
        return std::make_unique<CmdGetSynapse>(server, fb_cmd, packet_data);
    }

    void execute() override
    {
        if (!fb_cmd)
            return;
        const auto* cmd = fb_cmd->t_as_CmdGetSynapse();
        if (!cmd)
            return;

        uint8_t core_id = cmd->core_id();
        u32 pre_neur_id = cmd->pre_neur_id();
        u32 post_neur_id = cmd->post_neur_id();
        Logger::debug(
            "Executing CmdGetSynapse: core_id=%d, pre_neur_id=%u, post_neur_id=%u", core_id, pre_neur_id, post_neur_id);

        auto& cores = server.get_odin_cores();
        if (core_id >= cores.size()) {
            Logger::warning("CmdGetSynapse: core_id %d out of range (max: %d)", core_id, (int)cores.size() - 1);
            return;
        }
        if (cores[core_id] == nullptr) {
            Logger::warning("CmdGetSynapse: core_id %d is null", core_id);
            return;
        }
        if (pre_neur_id >= 256) {
            Logger::warning("CmdGetSynapse: pre_neur_id %d out of range (max: 255)", pre_neur_id);
            return;
        }
        if (post_neur_id >= 256) {
            Logger::warning("CmdGetSynapse: post_neur_id %d out of range (max: 255)", post_neur_id);
            return;
        }
        const u8 weight = cores[core_id]->get_synapse(pre_neur_id, post_neur_id);

        const bool ok = send_cmd_output([&]() {
            FB::CmdOutput_tUnion output_union;
            FB::CmdOutputGetSynapseT output_payload;
            output_payload.core_id = core_id;
            output_payload.pre_neur_id = pre_neur_id;
            output_payload.post_neur_id = post_neur_id;
            output_payload.weight = weight;

            output_union.Set(std::move(output_payload));
            return output_union;
        });

        if (!ok) {
            Logger::warning("CmdGetSynapse: Failed to send response for core %u pre %u post %u",
                            static_cast<unsigned>(core_id),
                            static_cast<unsigned>(pre_neur_id),
                            static_cast<unsigned>(post_neur_id));
        }
    }
};
