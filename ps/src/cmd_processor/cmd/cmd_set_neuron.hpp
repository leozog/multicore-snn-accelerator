#pragma once
#include "cmd.hpp"
#include "odin/odin_neuron.hpp"

class CmdSetNeuron : public Cmd
{
private:
    static ODIN_neuron fb_to_odin_neuron(const FB::NeuronWords128* fb_neuron)
    {
        ODIN_neuron neuron{};
        if (!fb_neuron)
            return neuron;

        auto* words_array = fb_neuron->words();
        neuron.words[0] = words_array->Get(0);
        neuron.words[1] = words_array->Get(1);
        neuron.words[2] = words_array->Get(2);
        neuron.words[3] = words_array->Get(3);

        return neuron;
    }

public:
    CmdSetNeuron(OdinServer& server, const FB::Cmd* fb_cmd, const std::shared_ptr<std::vector<uint8_t>>& packet_data)
        : Cmd(server, fb_cmd, packet_data)
    {
    }

    static std::unique_ptr<CmdSetNeuron> from_flatbuffer(OdinServer& server,
                                                         const FB::Cmd* fb_cmd,
                                                         const std::shared_ptr<std::vector<uint8_t>>& packet_data)
    {
        if (!fb_cmd || fb_cmd->t_type() != FB::Cmd_t_CmdSetNeuron)
            return nullptr;
        return std::make_unique<CmdSetNeuron>(server, fb_cmd, packet_data);
    }

    void execute() override
    {
        if (!fb_cmd)
            return;
        const auto* cmd = fb_cmd->t_as_CmdSetNeuron();
        if (!cmd)
            return;

        uint8_t core_id = cmd->core_id();
        u32 neur_id = cmd->neur_id();
        Logger::debug("Executing CmdSetNeuron: core_id=%d, neur_id=%u", core_id, neur_id);

        auto& cores = server.get_odin_cores();
        if (core_id >= cores.size()) {
            Logger::warning("CmdSetNeuron: core_id %d out of range (max: %d)", core_id, (int)cores.size() - 1);
            return;
        }
        if (cores[core_id] == nullptr) {
            Logger::warning("CmdSetNeuron: core_id %d is null", core_id);
            return;
        }
        if (neur_id >= 256) {
            Logger::warning("CmdSetNeuron: neur_id %d out of range (max: 255)", neur_id);
            return;
        }
        const auto* neuron_words = cmd->neuron_words();
        if (!neuron_words) {
            Logger::warning("CmdSetNeuron: neuron_words struct is null");
            return;
        }

        auto odin_neuron = fb_to_odin_neuron(neuron_words);
        cores[core_id]->set_neuron(neur_id, odin_neuron);

        send_handle_response();
    }
};
