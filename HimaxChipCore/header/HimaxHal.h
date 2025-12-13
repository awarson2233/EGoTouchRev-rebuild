// HimaxHal.h
#pragma once
#include <cstdint>
#include <mutex>
#include <vector>
#include <windows.h>

namespace Himax {
    enum class DeviceType { Master, Slave, Interrupt };

    class HalDevice {
    public:
        HalDevice(const wchar_t* path, DeviceType type);
        ~HalDevice();

        bool IsValid() const;
        bool Ioctl(DWORD code, const void* in, uint32_t inLen, void* out,
                   uint32_t outLen, uint32_t* retLen);
        bool Read(void* buffer, uint32_t len);
        bool WaitInterrupt();
        bool ReadBus(uint8_t cmd, uint8_t* data, uint32_t len);
        bool WriteBus(const uint8_t cmd, const uint8_t* addr, const uint8_t* data, uint32_t len);
        bool ReadAcpi(uint8_t* data, uint32_t len);
        bool GetFrame(void* buffer, uint32_t outLen, uint32_t* retLen);
        bool SetTimeOut(uint8_t millisecond);
        bool SetBlock(bool status);
        bool SetReset(bool state);
        bool IntOpen(void);
        bool IntClose(void);
        DWORD GetError(void);

    private:
        HANDLE m_handle;
        DWORD m_lastError;
        DeviceType m_type;

        uint8_t m_readOp;
        uint8_t m_writeOp;

        const size_t dummy_size = 1;
        const size_t header_size = 2;
        const size_t data_offset = header_size + dummy_size;

        std::vector<uint8_t> m_xfer_buffer;
        std::recursive_mutex m_mutex;
    };
}