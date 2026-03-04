#pragma once

#include "portmacro.h"
#include <functional>
#include <type_traits>
extern "C"
{
#include "FreeRTOS.h"
#include "queue.h"
}

template<typename T>
class RtosQueue
{
    static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
    QueueHandle_t handle;
    std::function<void(T&&)> item_deleter;

public:
    RtosQueue(UBaseType_t queue_length, std::function<void(T&&)> deleter = nullptr)
        : handle(xQueueCreate(queue_length, sizeof(T)))
        , item_deleter(deleter)
    {
    }

    ~RtosQueue()
    {
        clear();
        if (handle != nullptr) {
            vQueueDelete(handle);
        }
        handle = nullptr;
    }

    explicit operator bool() const { return handle != nullptr; }

    bool empty() const { return (handle == nullptr) || (uxQueueMessagesWaiting(handle) == 0); }

    UBaseType_t messages_waiting() const { return (handle == nullptr) ? 0 : uxQueueMessagesWaiting(handle); }

    UBaseType_t spaces_available() const { return (handle == nullptr) ? 0 : uxQueueSpacesAvailable(handle); }

    UBaseType_t messages_waiting_from_isr() const
    {
        return (handle == nullptr) ? 0 : uxQueueMessagesWaitingFromISR(handle);
    }

    UBaseType_t is_full_from_isr() const { return (handle == nullptr) ? 0 : xQueueIsQueueFullFromISR(handle); }

    bool push(const T& item, TickType_t timeout_ticks = 0) const
    {
        return xQueueSend(handle, &item, timeout_ticks) == pdPASS;
    }

    bool push_back(const T& item, TickType_t timeout_ticks = 0) const
    {
        return xQueueSendToBack(handle, &item, timeout_ticks) == pdPASS;
    }

    bool push_front(const T& item, TickType_t timeout_ticks = 0) const
    {
        return xQueueSendToFront(handle, &item, timeout_ticks) == pdPASS;
    }

    bool push_from_isr(const T& item, BaseType_t* higher_priority_task_woken) const
    {
        return xQueueSendFromISR(handle, &item, higher_priority_task_woken) == pdPASS;
    }

    bool pop(T* out_item, TickType_t timeout_ticks) const
    {
        return xQueueReceive(handle, out_item, timeout_ticks) == pdPASS;
    }

    bool pop_from_isr(T* out_item, BaseType_t* higher_priority_task_woken) const
    {
        return xQueueReceiveFromISR(handle, out_item, higher_priority_task_woken) == pdPASS;
    }

    bool peek(T* out_item, TickType_t timeout_ticks = 0) const
    {
        return xQueuePeek(handle, out_item, timeout_ticks) == pdPASS;
    }

    bool peek_from_isr(T* out_item) const { return xQueuePeekFromISR(handle, out_item) == pdPASS; }

    void clear()
    {
        if (handle == nullptr) {
            return;
        }

        T item{};
        while (xQueueReceive(handle, &item, 0) == pdPASS) {
            if (item_deleter) {
                item_deleter(std::move(item));
            }
        }

        if (handle != nullptr) {
            xQueueReset(handle);
        }
    }

    QueueHandle_t get() const { return handle; }
};
