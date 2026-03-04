#pragma once
extern "C"
{
#include "FreeRTOS.h"
#include "semphr.h"
#include "xil_types.h"
}
#include "rtos_mutex_guard.hpp"
#include <map>
#include <memory>
#include <vector>

using event_addr_t = u32;
using core_id_t = u8;
using neuron_id_t = u8;
using packet_t = std::vector<std::unique_ptr<std::vector<u32>>>;

class Routes
{
public:
    struct Event
    {
        event_addr_t addr;
        core_id_t to_core;
    };

    struct NeuronEventRoute
    {
        std::vector<Event> event_routes;
        bool server_output;
    };

    struct CoreNeuronRoute
    {
        std::vector<NeuronEventRoute> neuron_routes;
    };

    std::vector<CoreNeuronRoute> core_routes;

    SemaphoreHandle_t routes_mutex;

    Routes(int num_cores, int neurons_per_core);

    void clear();
    void add_route(core_id_t from_core, neuron_id_t from_neur, core_id_t to_core, event_addr_t event_addr);
    void set_server_output(core_id_t from_core, neuron_id_t from_neur, bool server_output);

    SemaphoreHandle_t get_mutex() const { return routes_mutex; }
};
