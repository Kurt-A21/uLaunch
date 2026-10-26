
#pragma once
// steamui: sys-clk's C client (vendored from retronx-team/sys-clk, see source/ul/menu/sysclk/LICENSE.sys-clk).
// Its headers multiply two different enums, which C++20+ warns about; the code is fine, so silence it here.
#include <switch.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
extern "C" {
#include <sysclk.h>
#include <sysclk/client/ipc.h>
}
#pragma GCC diagnostic pop
