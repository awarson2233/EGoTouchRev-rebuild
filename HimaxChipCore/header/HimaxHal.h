#include "windows.h"
#include <cstdint>
#include <memory>
#include <string>
#include <winnt.h>

namespace Himax {
    enum class DeviceType { Master, Slave, Interrupt };

    namespace Proctocol {
        constexpr uint8_t OP_WRITE_MASTER = 0xF2;
        constexpr uint8_t OP_READ_MASTER  = 0xF3;
        constexpr uint8_t OP_WRITE_SLAVE  = 0xF4;
        constexpr uint8_t OP_READ_SLAVE   = 0xF5;
    }

    class SPBControl {
    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;

        DeviceType m_type;
        uint8_t m_readOp;
        uint8_t m_writeOp;

    public:
        SPBControl(const std::wstring& devicePath, DeviceType type);
        ~SPBControl();

        bool IsValid() const;

        bool WriteRegister(uint8_t* reg, uint32_t regLen, uint8_t* data, uint32_t dataLen);
        bool ReadRegister(uint8_t *cmdd, uint8_t *buf, uint32_t len);
        bool ReadAcpi();
        bool WaitInterrupt();
        bool SetReset();
        bool SetBlock();
        bool SetTimeout();
        bool GetFrame();
    };
}