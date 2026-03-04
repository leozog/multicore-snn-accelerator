#pragma once
extern "C"
{
#include "xil_types.h"
}
#include <vector>

using core_id_t = u8;

struct RouterInput
{
    std::vector<u8>* buffer_ptr;
    bool last;
    core_id_t core_id;
};
