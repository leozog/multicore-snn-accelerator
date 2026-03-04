#pragma once

extern "C"
{
#include "xil_types.h"
}

#include "odin.hpp"

namespace odin_test {
bool run_full_rw_test(UINTPTR odin_baseaddr);
bool run_open_loop_lif_test(ODIN<256, 8>& odin_core);
}
