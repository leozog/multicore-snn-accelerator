#pragma once
#include "cmd_processor/cmd_processor.hpp"
#include "event_router/event_router.hpp"
#include "event_router/router_input.hpp"
#include "logger.hpp"
#include "lwip/inet.h"
#include "lwip/ip_addr.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/tcp.h"
#include "lwipopts.h"
#include "odin/odin.hpp"
#include "odin_server.h"
#include "odin_server_config.h"
#include "rtos_mutex_guard.hpp"
#include "rtos_queue.hpp"
#include "xil_printf.h"
#include "xil_types.h"
#include <memory>
#include <vector>

extern "C"
{
#include "FreeRTOS.h"
#include "semphr.h"
}

#undef bind
#undef socket
#undef accept
#undef listen
#undef connect

class OdinServer
{
    RtosQueue<RouterInput> event_router_input_queue;
    std::vector<std::unique_ptr<ODIN<256, 8>>> odin_cores;
    std::unique_ptr<EventRouter> event_router;
    std::unique_ptr<CmdProcessor> cmd_processor;

    int listen_sock;
    int client_sock;
    SemaphoreHandle_t client_sock_mutex;

public:
    OdinServer();
    OdinServer(const OdinServer&) = delete;
    OdinServer& operator=(const OdinServer&) = delete;
    OdinServer(OdinServer&&) = delete;
    OdinServer& operator=(OdinServer&&) = delete;

    void start();

    std::vector<std::unique_ptr<ODIN<256, 8>>>& get_odin_cores() { return odin_cores; }
    EventRouter& get_event_router();
    CmdProcessor& get_cmd_processor();
    bool send_output_packet(const std::vector<u8>& payload);

private:
    bool recv_all(u8* data, u32 size);
    bool send_all(const u8* data, u32 size);
    void init_odin_cores(RtosQueue<RouterInput>& event_router_input_queue);
    void init_listen_sock();
    void tcp_server_task();
};