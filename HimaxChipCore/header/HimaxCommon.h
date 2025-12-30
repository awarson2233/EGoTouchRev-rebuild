#pragma once

#include <expected>
#include <string_view>
#include <string>
#include <format>

namespace Himax {

enum class ChipError {
    Success = 0,
    BusError,           // I2C/SPI Bus read/write failed
    Timeout,            // Operation timed out
    VerificationFailed, // Readback value does not match written value
    InvalidParam,       // Invalid parameter provided
    SafeModeFailed,     // Failed to enter/exit safe mode
    FirmwareError,      // Firmware status is abnormal
    NotReady,           // Device is not ready
    Unknown             // Unknown error
};

constexpr std::string_view ErrorToString(ChipError err) {
    switch(err) {
        case ChipError::Success: return "Success";
        case ChipError::BusError: return "Bus Error";
        case ChipError::Timeout: return "Timeout";
        case ChipError::VerificationFailed: return "Verification Failed";
        case ChipError::InvalidParam: return "Invalid Parameter";
        case ChipError::SafeModeFailed: return "Safe Mode Failed";
        case ChipError::FirmwareError: return "Firmware Error";
        case ChipError::NotReady: return "Not Ready";
        case ChipError::Unknown: return "Unknown";
        default: return "Unknown";
    }
}

template<typename T>
using ChipResult = std::expected<T, ChipError>;

} // namespace Himax
