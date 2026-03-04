#pragma once

#include "rtos_queue.hpp"
#include <memory>
#include <queue>
#include <vector>

class OdinServer;
class Cmd;
namespace FB {
struct Cmd;
}

class CmdProcessor
{
    OdinServer& server;
    TaskHandle_t cmd_processor_task_handle;
    RtosQueue<Cmd*> cmd_queue;

public:
    CmdProcessor(OdinServer& server);
    CmdProcessor(const CmdProcessor&) = delete;
    CmdProcessor& operator=(const CmdProcessor&) = delete;

    void submit_cmd_packet(std::shared_ptr<std::vector<uint8_t>>&& packet_data);

    void clear_pending_commands();

private:
    std::unique_ptr<Cmd> cmd_from_flatbuffer(const FB::Cmd* fb_cmd,
                                             const std::shared_ptr<std::vector<uint8_t>>& packet_data);
    void cmd_processor_task();
    void process_cmd(std::unique_ptr<Cmd>&& cmd);
};
