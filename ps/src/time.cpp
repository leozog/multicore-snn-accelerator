#include "time.hpp"
#include <optional>
extern "C"
{
#include "xtime_l.h"
}

u64 get_time_ms()
{
    static std::optional<XTime> start_count = std::nullopt;

    XTime now_count = 0;
    XTime_GetTime(&now_count);

    if (!start_count.has_value()) {
        start_count = now_count;
    }

    const XTime elapsed_counts = now_count - start_count.value();

    return (static_cast<u64>(elapsed_counts) * 1000ULL) / static_cast<u64>(COUNTS_PER_SECOND);
}
