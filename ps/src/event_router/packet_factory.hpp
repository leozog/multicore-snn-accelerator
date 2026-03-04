#pragma once
extern "C"
{
#include "xil_types.h"
}
#include "event_router/routes.hpp"
#include <map>
#include <memory>
#include <vector>

class PacketFactory
{
    const Routes& routes;
    const int max_tx_buffer_len;

    std::vector<packet_t> packet_for_core;

public:
    PacketFactory(const Routes& routes, int max_tx_buffer_len);

    void parse_event(const event_addr_t event_addr, const core_id_t from_core);

    packet_t get_packet_for_core(core_id_t core_id);

private:
    void add_to_packet(event_addr_t event_addr, core_id_t to_core);
};