#pragma once

#include <jstm/types.hpp>

namespace jstm::platform {

#if defined(STM32F746xx) || defined(STM32F7)
inline constexpr bool is_f7 = true;
#else
inline constexpr bool is_f7 = false;
#endif

#if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1)
inline constexpr bool has_fpu = true;
#else
inline constexpr bool has_fpu = false;
#endif

#if defined(STM32F746xx)
inline constexpr bool has_ethernet = true;
#else
inline constexpr bool has_ethernet = false;
#endif

inline constexpr int core_count = 1;

#if defined(STM32F746xx)
inline constexpr u32 clock_mhz = 216;
#else
inline constexpr u32 clock_mhz = 0;
#endif

}  // namespace jstm::platform
