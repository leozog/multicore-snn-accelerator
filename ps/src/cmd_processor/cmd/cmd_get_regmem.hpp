#pragma once
#include "cmd.hpp"
#include "odin/odin_mem.hpp"
#include <array>

class CmdGetRegmem : public Cmd
{
public:
    CmdGetRegmem(OdinServer& server, const FB::Cmd* fb_cmd, const std::shared_ptr<std::vector<uint8_t>>& packet_data)
        : Cmd(server, fb_cmd, packet_data)
    {
    }

    static std::unique_ptr<CmdGetRegmem> from_flatbuffer(OdinServer& server,
                                                         const FB::Cmd* fb_cmd,
                                                         const std::shared_ptr<std::vector<uint8_t>>& packet_data)
    {
        if (!fb_cmd || fb_cmd->t_type() != FB::Cmd_t_CmdGetRegmem)
            return nullptr;
        return std::make_unique<CmdGetRegmem>(server, fb_cmd, packet_data);
    }

    void execute() override
    {
        if (!fb_cmd)
            return;
        const auto* cmd = fb_cmd->t_as_CmdGetRegmem();
        if (!cmd)
            return;

        uint8_t core_id = cmd->core_id();
        Logger::debug("Executing CmdGetRegmem: core_id=%d", core_id);

        auto& cores = server.get_odin_cores();
        if (core_id >= cores.size()) {
            Logger::warning("CmdGetRegmem: core_id %d out of range (max: %d)", core_id, (int)cores.size() - 1);
            return;
        }
        if (cores[core_id] == nullptr) {
            Logger::warning("CmdGetRegmem: core_id %d is null", core_id);
            return;
        }
        auto regmem = cores[core_id]->get_regmem();

        const bool ok = send_cmd_output([&]() {
            FB::CmdOutput_tUnion output_union;
            FB::CmdOutputGetRegmemT output_payload;
            output_payload.core_id = core_id;

            std::array<uint32_t, 8> syn_sign_arr = { regmem.syn_sign[0], regmem.syn_sign[1], regmem.syn_sign[2],
                                                     regmem.syn_sign[3], regmem.syn_sign[4], regmem.syn_sign[5],
                                                     regmem.syn_sign[6], regmem.syn_sign[7] };
            std::array<uint32_t, 8> reserved_syn_sign_arr = {
                regmem.reserved_syn_sign[0], regmem.reserved_syn_sign[1], regmem.reserved_syn_sign[2],
                regmem.reserved_syn_sign[3], regmem.reserved_syn_sign[4], regmem.reserved_syn_sign[5],
                regmem.reserved_syn_sign[6], regmem.reserved_syn_sign[7]
            };

            const uint32_t open_loop = regmem.open_loop;
            const uint32_t burst_timeref = regmem.burst_timeref;
            const uint32_t aer_src_ctrl_nneur = regmem.aer_src_ctrl_nneur;
            const uint32_t out_aer_monitor_en = regmem.out_aer_monitor_en;
            const uint32_t monitor_neur_addr = regmem.monitor_neur_addr;
            const uint32_t monitor_syn_addr = regmem.monitor_syn_addr;
            const uint32_t update_unmapped_syn = regmem.update_unmapped_syn;
            const uint32_t propagate_unmapped_syn = regmem.propagate_unmapped_syn;
            const uint32_t sdsp_on_syn_stim = regmem.sdsp_on_syn_stim;

            output_payload.regmem =
                std::make_unique<FB::OdinRegmem>(open_loop,
                                                 flatbuffers::span<const uint32_t, 8>(syn_sign_arr),
                                                 flatbuffers::span<const uint32_t, 8>(reserved_syn_sign_arr),
                                                 burst_timeref,
                                                 aer_src_ctrl_nneur,
                                                 out_aer_monitor_en,
                                                 monitor_neur_addr,
                                                 monitor_syn_addr,
                                                 update_unmapped_syn,
                                                 propagate_unmapped_syn,
                                                 sdsp_on_syn_stim);

            output_union.Set(std::move(output_payload));
            return output_union;
        });

        if (!ok) {
            Logger::warning("CmdGetRegmem: Failed to send response for core %u", static_cast<unsigned>(core_id));
        }
    }
};
