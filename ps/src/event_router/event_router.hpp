#pragma once
#include "xil_types.h"
#include <functional>
#include <memory>
extern "C"
{
#include "FreeRTOS.h"
#include "task.h"
}
#include "../rtos_queue.hpp"
#include "logger.hpp"
#include "odin/odin.hpp"
#include "router_input.hpp"
#include "routes.hpp"

class EventRouter
{
public:
    struct Cfg
    {
        RtosQueue<RouterInput>& input_queue;
        std::vector<std::unique_ptr<ODIN<256, 8>>>& odin_cores;
        u32 neurons_per_core;
        u32 max_tx_buffer_len;
        using send_output_callback_t = std::function<bool(const std::vector<u8>&)>;
        send_output_callback_t send_output_callback;
    };

private:
    Cfg cfg;

    TaskHandle_t loop_task_handle;

public:
    EventRouter(const Cfg& cfg);
    EventRouter(const EventRouter&) = delete;
    EventRouter& operator=(const EventRouter&) = delete;
    EventRouter(EventRouter&&) = delete;
    EventRouter& operator=(EventRouter&&) = delete;

    Routes routing_table;

    enum class State
    {
        IDLE,
        BUSY
    };
    State get_state() const;
    void reset();

private:
    void event_router_task();
    void send_output(core_id_t core_id, const std::vector<u8>& words);

    void router_error(const char* msg);
};
