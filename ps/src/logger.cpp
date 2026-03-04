#include "logger.hpp"
extern "C"
{
#include "xil_printf.h"
}
#include "cmd_processor/flatbuffers/output_generated.h"
#include "flatbuffers/flatbuffer_builder.h"
#include "rtos_mutex_guard.hpp"
#include "time.hpp"
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <utility>
#include <vector>

bool Logger::debug_output = true;
bool Logger::uart_output = true;
SemaphoreHandle_t Logger::mutex = nullptr;
Logger::send_output_callback_t Logger::send_output_callback = nullptr;

void Logger::set_send_output_callback(Logger::send_output_callback_t send_output_callback)
{
    Logger::send_output_callback = send_output_callback;
}

void Logger::init(bool debug_output, bool uart_output)
{
    Logger::debug_output = debug_output;
    Logger::uart_output = uart_output;

    mutex = xSemaphoreCreateMutex();
    if (mutex == nullptr) {
        xil_printf("Logger: Failed to create mutex\r\n");
        error_stop();
    }

    info_force_uart("Logger initialized");
    info_force_uart("DEBUG_LOGGING = %s", Logger::debug_output ? "true" : "false");
    info_force_uart("UART_LOGGING = %s", Logger::uart_output ? "true" : "false");
}

bool Logger::is_debug()
{
    return Logger::debug_output;
}

void Logger::info(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(MessageType::INFO, uart_output, true, format, args);
    va_end(args);
}

void Logger::info_force_uart(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(MessageType::INFO, true, true, format, args);
    va_end(args);
}

void Logger::info_only_uart(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(MessageType::INFO, true, false, format, args);
    va_end(args);
}

void Logger::warning(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(MessageType::WARNING, uart_output, true, format, args);
    va_end(args);
}

void Logger::warning_force_uart(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(MessageType::WARNING, true, true, format, args);
    va_end(args);
}

void Logger::warning_only_uart(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(MessageType::WARNING, true, false, format, args);
    va_end(args);
}

void Logger::error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(MessageType::ERROR, true, true, format, args);
    va_end(args);
}

void Logger::error_force_uart(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(MessageType::ERROR, true, true, format, args);
    va_end(args);
}

void Logger::error_only_uart(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(MessageType::ERROR, true, false, format, args);
    va_end(args);
}

void Logger::debug(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(MessageType::DEBUG, uart_output, true, format, args);
    va_end(args);
}

void Logger::debug_force_uart(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(MessageType::DEBUG, true, true, format, args);
    va_end(args);
}

void Logger::debug_only_uart(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(MessageType::DEBUG, true, false, format, args);
    va_end(args);
}

const char* Logger::get_type_string(MessageType type)
{
    switch (type) {
        case MessageType::INFO:
            return "INFO: ";
        case MessageType::WARNING:
            return "WARN: ";
        case MessageType::ERROR:
            return "ERROR:";
        case MessageType::DEBUG:
            return "DEBUG:";
        default:
            return "UNKNOWN:";
    }
}

void Logger::vlog(MessageType type, bool use_uart, bool use_send, const char* format, va_list args)
{
    if (type == MessageType::DEBUG && !Logger::debug_output) {
        return;
    }

    char message_buffer[512];
    va_list args_copy;
    va_copy(args_copy, args);
    std::vsnprintf(message_buffer, sizeof(message_buffer), format, args_copy);
    va_end(args_copy);

    std::uint64_t elapsed_ms = get_time_ms();
    char send_buffer[542];
    std::snprintf(send_buffer,
                  sizeof(send_buffer),
                  "[%lu ms] %s %s\r\n",
                  static_cast<unsigned long>(elapsed_ms),
                  Logger::get_type_string(type),
                  message_buffer);

    if (use_uart) {
        RtosMutexGuard guard(mutex);
        xil_printf("%s", send_buffer);
    }

    if (use_send && Logger::send_output_callback != nullptr) {
        send_output(type, send_buffer);
    }
}

void Logger::send_output(MessageType type, const char* text)
{
    if (text == nullptr || text[0] == '\0') {
        return;
    }

    FB::OutputT output;
    auto message = std::make_unique<FB::MessageT>();
    message->type = static_cast<FB::MessageType>(static_cast<u8>(type));
    message->text = text;
    output.message = std::move(message);

    flatbuffers::FlatBufferBuilder builder(512);
    const auto output_off = FB::CreateOutput(builder, &output);
    FB::FinishOutputBuffer(builder, output_off);

    const std::vector<u8> payload(builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize());
    if (!Logger::send_output_callback(payload)) {
        xil_printf("Logger: Failed to send log message output packet\r\n");
    }
}

void error_stop()
{
    volatile int i = 0;
    while (true) {
        i = i + 1;
    }
}
