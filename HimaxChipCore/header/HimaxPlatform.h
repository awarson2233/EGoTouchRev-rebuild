#pragma once
#include <cstddef>
#include <cstdint>
#include <minwindef.h>
#include <string>
#include <vector>
#include <winnt.h>
#include <winscard.h>

using SPI_HANDLE = void*;

namespace Himax {
    enum class DeviceType : std::uint8_t {
        Master = 0,
        Slave,
        Interrupt
    };

    namespace HalInternal {

    extern "C" {
        typedef void* SPI_HANDLE;
    
        /*** 基础管理 ***/
        SPI_HANDLE Spi_Open(const wchar_t* devicePath);
        void Spi_Close(SPI_HANDLE handle);
        bool Spi_IsOpen(SPI_HANDLE handle);
        uint32_t Spi_GetLastError(SPI_HANDLE handle);
    
        /*** 读写操作 ***/
        int Spi_ReadBus(SPI_HANDLE handle, uint8_t opCode, uint8_t cmd, uint8_t*data,uint8_t len);
        int Spi_WriteBus(SPI_HANDLE handle, uint8_t opCode, const uint8_t* payload, const uint32_t pLen);
        int Spi_GetFrame(SPI_HANDLE handle, void* buffer, uint32_t len, uint32_t* retLen);
        
        /*** 中断操作 ***/
        int Spi_IntOpen(SPI_HANDLE handle);
        int Spi_IntClose(SPI_HANDLE handle);
        int Spi_WaitInterrupt(SPI_HANDLE handle);
    
        /*** 基本操作 ***/
        int Spi_SetReset(SPI_HANDLE handle);
        int Spi_SetBlock(SPI_HANDLE handle, bool status);
        int Spi_SetTimeOut(SPI_HANDLE handle, uint8_t milisecond);
        }
    }

    namespace Proctocol {
        constexpr uint8_t CMD_MASTER_WRITE = 0xF2;
        constexpr uint8_t CMD_MASTER_READ = 0xF3;
        constexpr uint8_t CMD_SLAVE_WRITE = 0xF4;
        constexpr uint8_t CMD_SLAVE_READ = 0xF5;
    };

    class HalDevice {
    private:
        SPI_HANDLE m_handle;
        DeviceType m_type;

        uint8_t m_readOp;
        uint8_t m_writeOp;
    
    public:
        HalDevice(const std::wstring& path, DeviceType type) : m_type(type), m_handle(HalInternal::Spi_Open(path.c_str())) {
            switch (type) {
            case Himax::DeviceType::Master:
                m_readOp = Proctocol::CMD_MASTER_READ;
                m_writeOp = Proctocol::CMD_MASTER_WRITE;
                break;
            case Himax::DeviceType::Slave:
                m_readOp = Proctocol::CMD_SLAVE_READ;
                m_writeOp = Proctocol::CMD_SLAVE_WRITE;
                break;
            default:
                m_readOp = 0x00;
                m_writeOp = 0x00;
                break;
                
            }
        }
        ~HalDevice() {
            if (m_handle) {
                HalInternal::Spi_Close(m_handle);
            }
        }

        HalDevice(const HalDevice&) = delete;
        HalDevice& operator = (const HalDevice&) = delete;
        HalDevice(HalDevice&& other) noexcept : m_handle((other.m_handle)) {
            other.m_handle = nullptr;
        }

        bool SetReset() {
            if (m_type == DeviceType::Interrupt) {
                return false;
            }
            return HalInternal::Spi_SetReset(m_handle);
        }

        bool SetBlock(bool status) {
            if (m_type == DeviceType::Interrupt) {
                return false;
            }
            return HalInternal::Spi_SetBlock(m_handle, status);
        }

        bool SetTimeOut(uint8_t milisecond) {
            if (m_type == DeviceType::Interrupt) {
                return false;
            }
            return HalInternal::Spi_SetTimeOut(m_handle, milisecond);
        }
        
        bool WriteRegister(const uint8_t* reg, const uint32_t regLen, const uint8_t *data, uint32_t dataLen) {
            if (m_type == DeviceType::Interrupt) {
                return false;
            }
            std::vector<uint8_t> payload;
            payload.clear();
            if (regLen == 1) {
                payload.insert(payload.end(), reg, reg + regLen);
                payload.insert(payload.end(), data, data + dataLen);
                return HalInternal::Spi_WriteBus(m_handle, m_writeOp, payload.data(), regLen + dataLen);
            } else if (regLen == 4) {
                payload.push_back(0);
                payload.insert(payload.end(), reg, reg + regLen);
                payload.insert(payload.end(), data, data + dataLen);
                return HalInternal::Spi_WriteBus(m_handle, m_writeOp, payload.data(), regLen + dataLen + 1);
            }
            return false; 
        }

        bool ReadRegister(uint8_t cmd, uint8_t* data, uint32_t len) {
            if (m_type == DeviceType::Interrupt) {
                return false;
            }
            return HalInternal::Spi_ReadBus(m_handle, m_readOp, cmd, data, len);
        }

        bool GetFrame(uint8_t* buffer, uint32_t len, uint32_t* retLen) {
            if (m_type == DeviceType::Interrupt) {
                return false;
            }
            return HalInternal::Spi_GetFrame(m_handle, buffer, len, retLen);
        }
        
        bool WaitInterrupt() {
            if (m_type != DeviceType::Interrupt) {
                return false;
            }
            return HalInternal::Spi_WaitInterrupt(m_handle);
        }
    };
}