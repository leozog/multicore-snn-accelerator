#pragma once
#include "cmd.hpp"
#include "odin/odin_mem.hpp"

class CmdSetRegmem : public Cmd
{
private:
    static ODIN_regmem<256, 8> fb_to_odin_regmem(const FB::OdinRegmem* fb_regmem)
    {
        ODIN_regmem<256, 8> regmem{};
        if (!fb_regmem)
            return regmem;

        regmem.gate_activity = 0;
        regmem.open_loop = fb_regmem->open_loop();

        // Copy syn_sign array (8 elements)
        auto* syn_sign_array = fb_regmem->syn_sign();
        if (syn_sign_array) {
            for (size_t i = 0; i < 8; ++i) {
                regmem.syn_sign[i] = syn_sign_array->Get(i);
            }
        }

        // Copy reserved_syn_sign array (8 elements)
        auto* reserved_syn_sign_array = fb_regmem->reserved_syn_sign();
        if (reserved_syn_sign_array) {
            for (size_t i = 0; i < 8; ++i) {
                regmem.reserved_syn_sign[i] = reserved_syn_sign_array->Get(i);
            }
        }

        regmem.burst_timeref = fb_regmem->burst_timeref();
        regmem.aer_src_ctrl_nneur = fb_regmem->aer_src_ctrl_nneur();
        regmem.out_aer_monitor_en = fb_regmem->out_aer_monitor_en();
        regmem.monitor_neur_addr = fb_regmem->monitor_neur_addr();
        regmem.monitor_syn_addr = fb_regmem->monitor_syn_addr();
        regmem.update_unmapped_syn = fb_regmem->update_unmapped_syn();
        regmem.propagate_unmapped_syn = fb_regmem->propagate_unmapped_syn();
        regmem.sdsp_on_syn_stim = fb_regmem->sdsp_on_syn_stim();

        regmem.sw_rst = 0;
        regmem.busy = 0;

        return regmem;
    }

public:
    CmdSetRegmem(OdinServer& server, const FB::Cmd* fb_cmd, const std::shared_ptr<std::vector<uint8_t>>& packet_data)
        : Cmd(server, fb_cmd, packet_data)
    {
    }

    static std::unique_ptr<CmdSetRegmem> from_flatbuffer(OdinServer& server,
                                                         const FB::Cmd* fb_cmd,
                                                         const std::shared_ptr<std::vector<uint8_t>>& packet_data)
    {
        if (!fb_cmd || fb_cmd->t_type() != FB::Cmd_t_CmdSetRegmem)
            return nullptr;
        return std::make_unique<CmdSetRegmem>(server, fb_cmd, packet_data);
    }

    void execute() override
    {
        if (!fb_cmd)
            return;
        const auto* cmd = fb_cmd->t_as_CmdSetRegmem();
        if (!cmd)
            return;

        uint8_t core_id = cmd->core_id();
        Logger::debug("Executing CmdSetRegmem: core_id=%d", core_id);

        auto& cores = server.get_odin_cores();
        if (core_id >= cores.size()) {
            Logger::warning("CmdSetRegmem: core_id %d out of range (max: %d)", core_id, (int)cores.size() - 1);
            return;
        }
        if (cores[core_id] == nullptr) {
            Logger::warning("CmdSetRegmem: core_id %d is null", core_id);
            return;
        }
        const auto* regmem = cmd->regmem();
        if (!regmem) {
            Logger::warning("CmdSetRegmem: regmem struct is null");
            return;
        }

        auto odin_regmem = fb_to_odin_regmem(regmem);
        cores[core_id]->set_regmem(odin_regmem);

        send_handle_response();
    }
};
