#pragma once
#include "Logger.hpp"
extern "C"
{
#include "FreeRTOS.h"
#include "semphr.h"
#include "xil_printf.h"
#include "xil_types.h"
}

class RtosMutexGuard
{
private:
    SemaphoreHandle_t mutex;
    bool acquired;

public:
    explicit RtosMutexGuard(SemaphoreHandle_t m, TickType_t timeout = portMAX_DELAY)
        : mutex(m)
        , acquired(false)
    {
        if (mutex == nullptr) {
            xil_printf("RtosMutexGuard: Provided mutex handle is null\r\n");
            error_stop();
        }

        if (xSemaphoreTake(mutex, timeout) == pdTRUE) {
            acquired = true;
        } else {
            xil_printf("RtosMutexGuard: Failed to acquire mutex\r\n");
            error_stop();
        }
    }

    ~RtosMutexGuard()
    {
        if (acquired && mutex != nullptr) {
            xSemaphoreGive(mutex);
        }
    }

    RtosMutexGuard(const RtosMutexGuard&) = delete;
    RtosMutexGuard& operator=(const RtosMutexGuard&) = delete;

    RtosMutexGuard(RtosMutexGuard&& other) noexcept
        : mutex(other.mutex)
        , acquired(other.acquired)
    {
        other.acquired = false;
    }

    RtosMutexGuard& operator=(RtosMutexGuard&& other) noexcept
    {
        if (this != &other) {
            if (acquired && mutex != nullptr) {
                xSemaphoreGive(mutex);
            }
            mutex = other.mutex;
            acquired = other.acquired;
            other.acquired = false;
        }
        return *this;
    }

    bool is_acquired() const { return acquired; }
};
