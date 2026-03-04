#include "routes.hpp"
#include "../logger.hpp"

extern "C"
{
#include "FreeRTOS.h"
#include "semphr.h"
}

Routes::Routes(int num_cores, int neurons_per_core)
    : routes_mutex(xSemaphoreCreateMutex())
{
    if (routes_mutex == nullptr) {
        Logger::error_force_uart("Failed to create routes mutex");
    }
    core_routes.resize(num_cores);
    for (auto& core : core_routes) {
        core.neuron_routes.resize(neurons_per_core);
    }
    clear();
}

void Routes::clear()
{
    RtosMutexGuard guard(routes_mutex);

    for (auto& core : core_routes) {
        for (auto& neur : core.neuron_routes) {
            neur.event_routes.clear();
            neur.server_output = false;
        }
    }
}

void Routes::add_route(core_id_t from_core, neuron_id_t from_neur, core_id_t to_core, event_addr_t event_addr)
{
    RtosMutexGuard guard(routes_mutex);

    if (from_core >= core_routes.size()) {
        Logger::warning("Routes add_route: from_core %u out of range", static_cast<unsigned>(from_core));
        return;
    }
    if (to_core >= core_routes.size()) {
        Logger::warning("Routes add_route: to_core %u out of range", static_cast<unsigned>(to_core));
        return;
    }
    if (from_neur >= core_routes[from_core].neuron_routes.size()) {
        Logger::warning("Routes add_route: from_neur %u out of range", static_cast<unsigned>(from_neur));
        return;
    }

    Event event{ .addr = event_addr, .to_core = to_core };
    core_routes[from_core].neuron_routes[from_neur].event_routes.push_back(event);
}

void Routes::set_server_output(core_id_t from_core, neuron_id_t from_neur, bool server_output)
{
    RtosMutexGuard guard(routes_mutex);

    if (from_core >= core_routes.size()) {
        Logger::warning("Routes set_server_output: from_core %u out of range", static_cast<unsigned>(from_core));
        return;
    }
    if (from_neur >= core_routes[from_core].neuron_routes.size()) {
        Logger::warning("Routes set_server_output: from_neur %u out of range", static_cast<unsigned>(from_neur));
        return;
    }

    core_routes[from_core].neuron_routes[from_neur].server_output = server_output;
}