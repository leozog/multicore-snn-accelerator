#include "cmd_processor.hpp"
#include "cmd/cmd_list.hpp"
#include "flatbuffers/input_generated.h"
#include "logger.hpp"
#include <memory>
extern "C"
{
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
}

CmdProcessor::CmdProcessor(OdinServer& server)
    : server(server)
    , cmd_queue(16384, [](Cmd* cmd) { delete cmd; })
{
    if (xTaskCreate([](void* p) { static_cast<CmdProcessor*>(p)->cmd_processor_task(); },
                    "CMD_PROCESSOR",
                    1024,
                    this,
                    DEFAULT_THREAD_PRIO,
                    &cmd_processor_task_handle) != pdPASS) {
        Logger::error_force_uart("Failed to create CMD_PROCESSOR task");
    }
}

void CmdProcessor::submit_cmd_packet(std::shared_ptr<std::vector<uint8_t>>&& packet_data)
{
    const FB::Input* input = FB::GetInput(packet_data->data());
    if (!input) {
        Logger::error("CmdProcessor: Failed to parse input");
        return;
    }

    if (input->commands()) {
        for (const FB::Cmd* fb_cmd : *input->commands()) {
            auto cmd = cmd_from_flatbuffer(fb_cmd, packet_data);
            if (!cmd) {
                Logger::error("CmdProcessor: Failed to create command");
                continue;
            }

            if (cmd->get_execution_mode() == Cmd::ExecutionMode::IMMEDIATE) {
                cmd->execute();
            } else {
                Cmd* c = cmd.release();
                while (!cmd_queue.push(c)) {
                    // Logger::warning("CmdProcessor: Command queue full, retrying");
                    taskYIELD();
                }
            }
        }
    }
}

void CmdProcessor::clear_pending_commands()
{
    cmd_queue.clear();
}

void CmdProcessor::cmd_processor_task()
{
    Logger::info_force_uart("CMD_PROCESSOR task started");
    Cmd* cmd = nullptr;
    while (true) {
        if (cmd_queue.pop(&cmd, portMAX_DELAY)) {
            process_cmd(std::unique_ptr<Cmd>(cmd));
        }
    }
}

void CmdProcessor::process_cmd(std::unique_ptr<Cmd>&& cmd)
{
    if (!cmd) {
        Logger::error("CmdProcessor: Null command");
        return;
    }
    cmd->execute();
}
