#pragma once
#include "cmd.hpp"
#include "odin/odin_neuron.hpp"
#include <array>

class CmdGetNeuron : public Cmd
{
public:
    CmdGetNeuron(OdinServer& server, const FB::Cmd* fb_cmd, const std::shared_ptr<std::vector<uint8_t>>& packet_data)
        : Cmd(server, fb_cmd, packet_data)
    {
    }

    static std::unique_ptr<CmdGetNeuron> from_flatbuffer(OdinServer& server,
                                                         const FB::Cmd* fb_cmd,
                                                         const std::shared_ptr<std::vector<uint8_t>>& packet_data)
    {
        if (!fb_cmd || fb_cmd->t_type() != FB::Cmd_t_CmdGetNeuron)
            return nullptr;
        return std::make_unique<CmdGetNeuron>(server, fb_cmd, packet_data);
    }

    void execute() override
    {
        if (!fb_cmd)
            return;
        const auto* cmd = fb_cmd->t_as_CmdGetNeuron();
        if (!cmd)
            return;

        uint8_t core_id = cmd->core_id();
        u32 neur_id = cmd->neur_id();
        Logger::debug("Executing CmdGetNeuron: core_id=%d, neur_id=%u", core_id, neur_id);

        auto& cores = server.get_odin_cores();
        if (core_id >= cores.size()) {
            Logger::warning("CmdGetNeuron: core_id %d out of range (max: %d)", core_id, (int)cores.size() - 1);
            return;
        }
        if (cores[core_id] == nullptr) {
            Logger::warning("CmdGetNeuron: core_id %d is null", core_id);
            return;
        }
        if (neur_id >= 256) {
            Logger::warning("CmdGetNeuron: neur_id %d out of range (max: 255)", neur_id);
            return;
        }
        auto neuron = cores[core_id]->get_neuron(neur_id);
        std::array<uint32_t, 4> words_arr = { neuron.words[0], neuron.words[1], neuron.words[2], neuron.words[3] };

        const bool ok = send_cmd_output([&]() {
            FB::CmdOutput_tUnion output_union;
            FB::CmdOutputGetNeuronT output_payload;
            output_payload.core_id = core_id;
            output_payload.neur_id = neur_id;
            output_payload.neuron_words =
                std::make_unique<FB::NeuronWords128>(flatbuffers::span<const uint32_t, 4>(words_arr));

            output_union.Set(std::move(output_payload));
            return output_union;
        });

        if (!ok) {
            Logger::warning("CmdGetNeuron: Failed to send response for core %u neur %u",
                            static_cast<unsigned>(core_id),
                            static_cast<unsigned>(neur_id));
        }
    }
};
