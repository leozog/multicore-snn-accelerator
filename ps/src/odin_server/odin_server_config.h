#ifndef ODIN_SERVER_CONFIG_H
#define ODIN_SERVER_CONFIG_H

#ifndef DEFAULT_IP_ADDRESS
#define DEFAULT_IP_ADDRESS    "169.254.5.1"
#endif

#ifndef DEFAULT_IP_MASK
#define DEFAULT_IP_MASK	      "255.255.0.0"
#endif

#ifndef DEFAULT_GW_ADDRESS
#define DEFAULT_GW_ADDRESS    "0.0.0.0"
#endif

#ifndef MAC_ETHERNET_ADDRESS
#define MAC_ETHERNET_ADDRESS  {0x00, 0x0a, 0x35, 0x00, 0x01, 0x02}
#endif

#ifndef TCP_CONN_PORT
#define TCP_CONN_PORT         5001
#endif

#ifndef COMMUNICATION_TIMEOUT
#define COMMUNICATION_TIMEOUT 5000 
#endif

#ifndef DEBUG_LOGGING
#define DEBUG_LOGGING         0
#endif

#ifndef UART_LOGGING
#define UART_LOGGING          1
#endif

#endif /* ODIN_SERVER_CONFIG_H */