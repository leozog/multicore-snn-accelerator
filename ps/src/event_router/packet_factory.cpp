#include "packet_factory.hpp"
#include <memory>

PacketFactory::PacketFactory(const Routes& routes, int max_tx_buffer_len)
    : routes(routes)
    , max_tx_buffer_len(max_tx_buffer_len)
    , packet_for_core(routes.core_routes.size())
{
}

void PacketFactory::parse_event(const u32 event_addr, const core_id_t from_core)
{
    const auto& core_routes = routes.core_routes[from_core];
    const auto& neuron_routes = core_routes.neuron_routes[event_addr];
    for (const auto& event_route : neuron_routes.event_routes) {
        add_to_packet(event_route.addr, event_route.to_core);
    }
}

packet_t PacketFactory::get_packet_for_core(core_id_t core_id)
{
    return std::move(packet_for_core[core_id]);
}

void PacketFactory::add_to_packet(event_addr_t event_addr, core_id_t to_core)
{
    auto& packet_for_dest_core = packet_for_core[to_core];

    if (packet_for_dest_core.empty() || packet_for_dest_core.back()->size() >= max_tx_buffer_len - 1) {
        packet_for_dest_core.emplace_back(std::make_unique<std::vector<u32>>());
        packet_for_dest_core.back()->reserve(max_tx_buffer_len);
    }

    auto& current_buffer = packet_for_dest_core.back();
    current_buffer->push_back(event_addr);
}