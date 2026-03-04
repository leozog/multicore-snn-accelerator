
#include "odin_server.hpp"
void OdinServer::init_listen_sock()
{
    if ((listen_sock = lwip_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        Logger::error("TCP server: Error creating Socket");
        return;
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(TCP_CONN_PORT);
    address.sin_addr.s_addr = INADDR_ANY;

    if (lwip_bind(listen_sock, (struct sockaddr*)&address, sizeof(address)) < 0) {
        Logger::error("TCP server: Unable to bind to port %d", TCP_CONN_PORT);
        close(listen_sock);
        return;
    }

    if (lwip_listen(listen_sock, 1) < 0) {
        Logger::error("TCP server: tcp_listen failed");
        close(listen_sock);
        return;
    }
}