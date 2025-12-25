#pragma once

// SIMD feature detection for HimaxChipCore.
// This header is intentionally lightweight and safe to include on any platform.

// On ARM64, Advanced SIMD (NEON) is part of the architecture.
// On ARM32, NEON availability depends on toolchain/flags.
#if defined(__aarch64__) || defined(_M_ARM64)
    #define HIMAX_HAS_NEON 1
    #include <arm_neon.h>
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
    #define HIMAX_HAS_NEON 1
    #include <arm_neon.h>
#else
    #define HIMAX_HAS_NEON 0
#endif
