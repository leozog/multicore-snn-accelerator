#include <cstddef>
extern "C"
{
#include "FreeRTOS.h"
}

void* operator new(std::size_t size) noexcept
{
    return pvPortMalloc(size);
}

void operator delete(void* ptr) noexcept
{
    vPortFree(ptr);
}

void* operator new[](std::size_t size) noexcept
{
    return pvPortMalloc(size);
}

void operator delete[](void* ptr) noexcept
{
    vPortFree(ptr);
}