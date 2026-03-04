#pragma once
#include <cstdarg>
#include <cstdint>
#include <functional>
extern "C"
{
#include "FreeRTOS.h"
#include "semphr.h"
}

class Logger
{
    static bool debug_output;
    static bool uart_output;

    static SemaphoreHandle_t mutex;

public:
    enum MessageType : std::uint8_t
    {
        INFO = 0,
        WARNING = 1,
        ERROR = 2,
        DEBUG = 3
    };

    static void init(bool debug_output, bool uart_output = false);

    static bool is_debug();

    static void info(const char* format, ...);
    static void info_force_uart(const char* format, ...);
    static void info_only_uart(const char* format, ...);
    static void warning(const char* format, ...);
    static void warning_force_uart(const char* format, ...);
    static void warning_only_uart(const char* format, ...);
    static void error(const char* format, ...);
    static void error_force_uart(const char* format, ...);
    static void error_only_uart(const char* format, ...);
    static void debug(const char* format, ...);
    static void debug_force_uart(const char* format, ...);
    static void debug_only_uart(const char* format, ...);

    using send_output_callback_t = std::function<bool(const std::vector<u8>&)>;
    static send_output_callback_t send_output_callback;
    static void set_send_output_callback(send_output_callback_t send_output_callback);

private:
    static std::function<bool(MessageType, const char*)> output_sender;
    static const char* get_type_string(MessageType type);
    static void vlog(MessageType type, bool use_uart, bool use_send, const char* format, va_list args);
    static void send_output(Logger::MessageType type, const char* text);
};

void error_stop();