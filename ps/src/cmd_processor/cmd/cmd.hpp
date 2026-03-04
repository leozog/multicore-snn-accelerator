#pragma once
#include "cmd_processor/flatbuffers/cmd_generated.h"
#include "cmd_processor/flatbuffers/cmd_output_generated.h"
#include "cmd_processor/flatbuffers/output_generated.h"
#include "logger.hpp"
class OdinServer;
#include <memory>
#include <utility>
extern "C"
{
#include "xil_types.h"
}

class Cmd
{
public:
    enum class ExecutionMode : u8
    {
        QUEUED,   // Execute from command queue
        IMMEDIATE // Execute immediately
    };

protected:
    OdinServer& server;
    const FB::Cmd* fb_cmd;
    const std::shared_ptr<std::vector<uint8_t>> packet_data;

    template<typename PayloadBuilder>
    bool send_cmd_output(PayloadBuilder&& payload_builder) const
    {
        if (!fb_cmd)
            return false;

        FB::CmdOutputT cmd_output;
        cmd_output.handle = static_cast<u32>(fb_cmd->handle());
        cmd_output.t = payload_builder();

        FB::OutputT output;
        output.cmd_output = std::make_unique<FB::CmdOutputT>(std::move(cmd_output));

        flatbuffers::FlatBufferBuilder builder(256);
        const auto output_off = FB::CreateOutput(builder, &output);
        FB::FinishOutputBuffer(builder, output_off);
        const std::vector<u8> payload(builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize());
        return server.send_output_packet(payload);
    }

public:
    Cmd(OdinServer& server, const FB::Cmd* fb_cmd, const std::shared_ptr<std::vector<uint8_t>>& packet_data)
        : server(server)
        , fb_cmd(fb_cmd)
        , packet_data(packet_data)
    {
    }
    virtual ~Cmd() = default;

    virtual ExecutionMode get_execution_mode() const { return ExecutionMode::QUEUED; }
    virtual void execute() = 0;

    void send_handle_response() const
    {
        const bool ok = send_cmd_output([]() {
            FB::CmdOutput_tUnion output_union;
            output_union.Set(FB::CmdOutputNoneT{});
            return output_union;
        });
        if (!ok) {
            Logger::warning("Cmd: Failed to send handle response");
        }
    }
};