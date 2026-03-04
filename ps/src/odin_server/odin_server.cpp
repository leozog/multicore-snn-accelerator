#include "odin_server.hpp"
#include "arch/sys_arch.h"
#include "cmd_processor/cmd_processor.hpp"
#include "cmd_processor/flatbuffers/output_generated.h"
#include "event_router/event_router.hpp"
#include "event_router/router_input.hpp"
#include "flatbuffers/flatbuffer_builder.h"
#include "logger.hpp"
#include "odin/odin.hpp"
#include "odin/odin_test.hpp"
#include "odin_server_config.h"
#include "rtos_mutex_guard.hpp"
#include "rtos_queue.hpp"
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_printf.h"
#include "xparameters.h"
#include <array>
#include <deque>
#include <utility>
#include <vector>

extern "C" void print_app_header(void)
{
    xil_printf("ODIN server listening on port %d\r\n", TCP_CONN_PORT);
}

extern "C" void start_application(void)
{
    Xil_DCacheFlush();
    Xil_DCacheDisable();

    Logger::init(DEBUG_LOGGING, UART_LOGGING);

    OdinServer server;
    server.start();
}

OdinServer::OdinServer()
    : event_router_input_queue(16318, [](RouterInput&& input) { delete input.buffer_ptr; })
    , listen_sock(-1)
    , client_sock(-1)
    , client_sock_mutex(xSemaphoreCreateMutex())
{
    Logger::info_force_uart("Starting the ODIN server");

    if (!event_router_input_queue) {
        Logger::error_force_uart("Failed to create EventRouter input queue");
        error_stop();
    }

    if (client_sock_mutex == nullptr) {
        Logger::error_force_uart("Failed to create client socket mutex");
        error_stop();
    }

    init_odin_cores(event_router_input_queue);

    EventRouter::Cfg er_cfg = {
        event_router_input_queue,
        odin_cores,
        256,
        odin_cores[0]->get_max_tx_buffer_len(), // Assume all cores have the same max tx buffer length
        [this](const std::vector<u8>& payload) { return this->send_output_packet(payload); }
    };
    event_router = std::make_unique<EventRouter>(er_cfg);

    cmd_processor = std::make_unique<CmdProcessor>(*this);

    Logger::set_send_output_callback(
        [this](const std::vector<u8>& payload) { return this->send_output_packet(payload); });
}

void OdinServer::start()
{
    init_listen_sock();
    tcp_server_task();
    // odin_test::run_open_loop_lif_test(*odin_cores[0]);
    // odin_test::run_open_loop_lif_test(*odin_cores[1]);
    // odin_test::run_open_loop_lif_test(*odin_cores[2]);
    // odin_test::run_open_loop_lif_test(*odin_cores[3]);
    // odin_test::run_open_loop_lif_test(*odin_cores[4]);
    // while (true) {
    //     Logger::debug_only_uart("odin state: %s",
    //                             odin_cores[0]->get_state() == ODIN<256, 8>::State::IDLE ? "IDLE" : "BUSY");
    //     vTaskDelay(100);
    // }
}

void OdinServer::tcp_server_task()
{
    Logger::info("TCP_SERVER task started");
    while (true) {
        struct sockaddr_in remote;
        socklen_t remote_size = sizeof(remote);
        if ((client_sock = lwip_accept(listen_sock, (struct sockaddr*)&remote, &remote_size)) > 0) {
            Logger::info_force_uart("TCP server: Client connected");

            while (true) {
                // Read the size prefix (4 bytes) little-endian
                uint32_t payload_size = 0;
                if (!recv_all(reinterpret_cast<u8*>(&payload_size), sizeof(payload_size))) {
                    Logger::info_force_uart("TCP server: Client disconnected");
                    break;
                }

                if (payload_size == 0) {
                    continue;
                }

                constexpr uint32_t MAX_PAYLOAD = 4 * 1024 * 1024; // 4 MiB
                if (payload_size > MAX_PAYLOAD) {
                    Logger::error("TCP server: incoming payload too large: %u", payload_size);
                    break;
                }

                auto packet_data = std::make_shared<std::vector<u8>>(payload_size);

                // Read the payload
                if (!recv_all(packet_data->data(), payload_size)) {
                    Logger::info_force_uart("TCP server: Client disconnected");
                    break;
                }

                cmd_processor->submit_cmd_packet(std::move(packet_data));
            }

            {
                RtosMutexGuard guard(client_sock_mutex); // Ensure we don't close the socket while sender is using it
                if (client_sock >= 0) {
                    close(client_sock);
                    client_sock = -1;
                }
            }
        }
    }
}

EventRouter& OdinServer::get_event_router()
{
    return *event_router;
}

CmdProcessor& OdinServer::get_cmd_processor()
{
    return *cmd_processor;
}

bool OdinServer::recv_all(u8* data, u32 size)
{
    size_t got = 0;
    while (got < size) {
        ssize_t ret = lwip_recv(client_sock, data + got, size - got, 0);
        if (ret <= 0) {
            return false;
        }
        got += static_cast<size_t>(ret);
    }
    return true;
}