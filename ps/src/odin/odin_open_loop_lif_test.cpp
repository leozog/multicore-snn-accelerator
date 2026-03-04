#include "../logger.hpp"
#include "odin_test.hpp"

extern "C"
{
#include "FreeRTOS.h"
#include "task.h"
}

#include <memory>
#include <vector>

namespace {
constexpr u32 EVENT_TAG_NEURON = 0x07u;
constexpr u32 TEST_TIMEOUT_TICKS = 1000u;

u32 mk_neuron_event(u8 neuron_idx)
{
    return (static_cast<u32>(neuron_idx) << 8) | EVENT_TAG_NEURON;
}

ODIN_neuron make_lif_neuron(u32 neur_idx, u8 threshold, u8 init_v, bool leak_en, u8 leak_str, bool neur_disable)
{
    (void)neur_idx;
    ODIN_neuron neuron{};
    neuron.words = { 0u, 0u, 0u, 0u };

    neuron.lif.lif_izh_sel = 1;
    neuron.lif.leak_str = leak_str & 0x7Fu;
    neuron.lif.leak_en = leak_en ? 1 : 0;
    neuron.lif.thr = threshold;
    neuron.lif.core = init_v;
    neuron.lif.neur_disable = neur_disable ? 1 : 0;

    return neuron;
}

u8 get_lif_threshold(const ODIN_neuron& neuron)
{
    return static_cast<u8>((neuron.words[0] >> 9) & 0xFFu);
}

bool wait_for_odin_idle(ODIN<256, 8>& odin_core, u32 timeout_ticks)
{
    for (u32 tick = 0; tick < timeout_ticks; ++tick) {
        if (odin_core.get_regmem().busy == 0u) {
            return true;
        }
        vTaskDelay(1);
    }
    return false;
}
}

namespace odin_test {
bool run_open_loop_lif_test(ODIN<256, 8>& odin_core)
{
    Logger::info_force_uart("ODIN open-loop LIF test: start");

    odin_core.zero_memory();

    auto regmem = odin_core.get_regmem();
    regmem.open_loop = 1u;
    odin_core.set_regmem(regmem);

    odin_core.set_neuron(0u, make_lif_neuron(0u, 5u, 0u, false, 0u, false));
    odin_core.set_neuron(1u, make_lif_neuron(1u, 4u, 0u, false, 0u, false));

    odin_core.set_synapse(10u, 0u, 0b1011u);
    odin_core.set_synapse(11u, 0u, 0b1011u);
    odin_core.set_synapse(10u, 1u, 0b1100u);

    if (get_lif_threshold(odin_core.get_neuron(0u)) != 5u) {
        Logger::error_force_uart("ODIN open-loop cfg readback: thr0=%u",
                                 static_cast<unsigned>(get_lif_threshold(odin_core.get_neuron(0u))));
        Logger::error_force_uart("ODIN open-loop LIF test: config readback mismatch");
        return false;
    }

    if (get_lif_threshold(odin_core.get_neuron(1u)) != 4u) {
        Logger::error_force_uart("ODIN open-loop cfg readback: thr1=%u",
                                 static_cast<unsigned>(get_lif_threshold(odin_core.get_neuron(1u))));
        Logger::error_force_uart("ODIN open-loop LIF test: config readback mismatch");
        return false;
    }

    if (odin_core.get_synapse(10u, 0u) != 0b1011u) {
        Logger::error_force_uart("ODIN open-loop cfg readback: syn10_0=0x%X",
                                 static_cast<unsigned>(odin_core.get_synapse(10u, 0u)));
        Logger::error_force_uart("ODIN open-loop LIF test: config readback mismatch");
        return false;
    }

    if (odin_core.get_synapse(11u, 0u) != 0b1011u) {
        Logger::error_force_uart("ODIN open-loop cfg readback: syn11_0=0x%X",
                                 static_cast<unsigned>(odin_core.get_synapse(11u, 0u)));
        Logger::error_force_uart("ODIN open-loop LIF test: config readback mismatch");
        return false;
    }

    if (odin_core.get_synapse(10u, 1u) != 0b1100u) {
        Logger::error_force_uart("ODIN open-loop cfg readback: syn10_1=0x%X",
                                 static_cast<unsigned>(odin_core.get_synapse(10u, 1u)));
        Logger::error_force_uart("ODIN open-loop LIF test: config readback mismatch");
        return false;
    }

    std::unique_ptr<std::vector<u32>> buffer = std::make_unique<std::vector<u32>>();
    for (u32 i = 0; i < 64; ++i) {
        buffer->push_back(mk_neuron_event(10u));
        buffer->push_back(mk_neuron_event(11u));
    }

    std::vector<std::unique_ptr<std::vector<u32>>> packet;
    packet.push_back(std::move(buffer));
    odin_core.send_packet(std::move(packet));

    if (!wait_for_odin_idle(odin_core, TEST_TIMEOUT_TICKS)) {
        Logger::error_force_uart("ODIN open-loop LIF test: timeout waiting for core idle");
        return false;
    }

    Logger::info_force_uart("ODIN open-loop LIF test: completed");
    return true;
}
}
