#include "cmd_processor/cmd_processor.hpp"
#include "cmd_processor/flatbuffers/output_generated.h"
#include "event_router/event_router.hpp"
#include "event_router/router_input.hpp"
#include "flatbuffers/flatbuffer_builder.h"
#include "logger.hpp"
#include "odin/odin.hpp"
#include "odin/odin_test.hpp"
#include "odin_server.hpp"
#include "odin_server_config.h"
#include "rtos_mutex_guard.hpp"
#include "rtos_queue.hpp"
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_printf.h"
#include "xinterrupt_wrap.h"
#include "xparameters.h"
#include <array>
#include <deque>
#include <utility>
#include <vector>

bool OdinServer::send_all(const u8* data, u32 size)
{
    u32 sent = 0;
    while (sent < size) {
        const u32 ret = lwip_send(this->client_sock, data + sent, size - sent, 0);
        if (ret <= 0) {
            return false;
        }
        sent += ret;
    }
    return true;
}

bool OdinServer::send_output_packet(const std::vector<u8>& payload)
{
    if (payload.empty()) {
        return false;
    }

    RtosMutexGuard guard(client_sock_mutex);

    if (this->client_sock < 0) {
        return true;
    }

    const u32 size_prefix = static_cast<u32>(payload.size());
    const bool ok = send_all(reinterpret_cast<const u8*>(&size_prefix), sizeof(size_prefix)) &&
                    send_all(payload.data(), static_cast<u32>(payload.size()));
    return ok;
}
