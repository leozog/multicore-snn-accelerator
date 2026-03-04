#include "event_router.hpp"
#include "../rtos_mutex_guard.hpp"
#include "cmd_processor/flatbuffers/output_generated.h"
#include "flatbuffers/flatbuffer_builder.h"
#include "packet_factory.hpp"
#include "xil_types.h"
#include <memory>
#include <utility>
#include <vector>

extern "C"
{
#include "FreeRTOS.h"
#include "queue.h"
}

EventRouter::EventRouter(const Cfg& cfg)
    : cfg(cfg)
    , loop_task_handle(nullptr)
    , routing_table(cfg.odin_cores.size(), cfg.neurons_per_core)
{
    if (xTaskCreate([](void* p) { static_cast<EventRouter*>(p)->event_router_task(); },
                    "EVENT_ROUTER",
                    1024,
                    this,
                    DEFAULT_THREAD_PRIO,
                    &loop_task_handle) != pdPASS) {
        router_error("Failed to create EVENT_ROUTER task");
    }
}

void EventRouter::event_router_task()
{
    Logger::info_force_uart("EVENT_ROUTER task started");
    while (true) {
        RouterInput input{};
        if (!cfg.input_queue.pop(&input, portMAX_DELAY)) {
            router_error("Failed to receive from input queue");
        }

        RtosMutexGuard guard(routing_table.get_mutex());

        auto input_buffer = std::unique_ptr<std::vector<u8>>(input.buffer_ptr);
        if (input_buffer == nullptr) {
            router_error("Received null input buffer");
            continue;
        }

        Logger::debug("EventRouter received input for core %u with buffer size %u",
                      static_cast<unsigned>(input.core_id),
                      static_cast<unsigned>(input_buffer->size()));

        // route events to cores
        PacketFactory packet_factory(routing_table, cfg.max_tx_buffer_len);
        for (auto& event : *input_buffer) {
            packet_factory.parse_event(event, input.core_id);
        }

        // send packets to cores
        for (core_id_t core_id = 0; core_id < cfg.odin_cores.size(); ++core_id) {
            auto packet = packet_factory.get_packet_for_core(core_id);
            if (!packet.empty()) {
                cfg.odin_cores[core_id]->send_packet(std::move(packet));
            }
        }

        // route events to client
        std::vector<u8> spikes_output;
        spikes_output.reserve(input_buffer->size());
        for (auto& event : *input_buffer) {
            if (routing_table.core_routes[input.core_id].neuron_routes[event].server_output) {
                spikes_output.push_back(event);
            }
        }

        // send output to client
        if (!spikes_output.empty()) {
            Logger::debug("EventRouter sending %u spikes to client from core %u", spikes_output.size(), input.core_id);
            send_output(input.core_id, spikes_output);
        }
    }
}

EventRouter::State EventRouter::get_state() const
{
    if (uxSemaphoreGetCount(routing_table.get_mutex()) == 0) {
        return State::BUSY;
    }
    return cfg.input_queue.messages_waiting() > 0 ? State::BUSY : State::IDLE;
}

void EventRouter::reset()
{
    Logger::debug_force_uart("EventRouter reset");
    routing_table.clear();
    cfg.input_queue.clear();
}

void EventRouter::send_output(core_id_t core_id, const std::vector<u8>& words)
{
    if (cfg.send_output_callback == nullptr || words.empty()) {
        return;
    }

    FB::OutputT output;
    auto spikes = std::make_unique<FB::SpikesT>();
    spikes->core_id = core_id;
    spikes->words = words;
    output.spikes = std::move(spikes);

    flatbuffers::FlatBufferBuilder builder(512);
    const auto output_off = FB::CreateOutput(builder, &output);
    FB::FinishOutputBuffer(builder, output_off);

    const std::vector<u8> payload(builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize());
    if (!cfg.send_output_callback(payload)) {
        Logger::warning_force_uart("EventRouter: Failed to send spikes output packet");
    }
}

void EventRouter::router_error(const char* msg)
{
    Logger::error_force_uart("EventRouter error: %s", msg);
    error_stop();
}
