#pragma once

constexpr unsigned long long clog2(unsigned long long x)
{
    return x == 1 ? 0 : 1 + clog2((x + 1) >> 1);
}

constexpr unsigned long long powof2(int x)
{
    return 1 << x;
}

constexpr unsigned long long ceildiv(unsigned long long x, unsigned long long y)
{
    return (x + y - 1) / y;
}