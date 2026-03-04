#pragma once
#include "cmd.hpp"
#include <sys/types.h>
#include <vector>

extern "C"
{
#include "FreeRTOS.h"
#include "task.h"
}

class CmdSendPacket : public Cmd
{
public:
    CmdSendPacket(OdinServer& server, const FB::Cmd* fb_cmd, const std::shared_ptr<std::vector<uint8_t>>& packet_data)
        : Cmd(server, fb_cmd, packet_data)
    {
    }

    static std::unique_ptr<CmdSendPacket> from_flatbuffer(OdinServer& server,
                                                          const FB::Cmd* fb_cmd,
                                                          const std::shared_ptr<std::vector<uint8_t>>& packet_data)
    {
        if (!fb_cmd || fb_cmd->t_type() != FB::Cmd_t_CmdSendPacket)
            return nullptr;
        return std::make_unique<CmdSendPacket>(server, fb_cmd, packet_data);
    }

    void execute() override
    {
        if (!fb_cmd)
            return;
        const auto* cmd = fb_cmd->t_as_CmdSendPacket();
        if (!cmd)
            return;

        uint8_t core_id = cmd->core_id();
        Logger::debug("Executing CmdSendPacket: core_id=%d", core_id);

        auto& cores = server.get_odin_cores();
        if (core_id >= cores.size()) {
            Logger::warning("CmdSendPacket: core_id %d out of range (max: %d)", core_id, (int)cores.size() - 1);
            return;
        }
        if (cores[core_id] == nullptr) {
            Logger::warning("CmdSendPacket: core_id %d is null", core_id);
            return;
        }
        const auto* words = cmd->words();
        if (!words || words->size() == 0) {
            Logger::warning("CmdSendPacket: No words to send");
            return;
        }

        const u32 max_buffer_len = cores[core_id]->get_max_tx_buffer_len() - 1; // Leave space for EOF
        if (words->size() < max_buffer_len) {
            auto buffer = std::make_unique<std::vector<u32>>();
            buffer->assign(words->begin(), words->end());
            cores[core_id]->send_packet(std::move(buffer));
        } else {
            auto packet = std::vector<std::unique_ptr<std::vector<u32>>>();
            u32 offset = 0;
            while (offset < words->size()) {
                auto buffer = std::make_unique<std::vector<u32>>();
                const u32 remaining = words->size() - offset;
                const u32 to_copy = (remaining < max_buffer_len) ? remaining : max_buffer_len;
                buffer->assign(words->begin() + offset, words->begin() + offset + to_copy);
                packet.push_back(std::move(buffer));
                offset += to_copy;
            }
            cores[core_id]->send_packet(std::move(packet));
        }

        send_handle_response();
    }
};
