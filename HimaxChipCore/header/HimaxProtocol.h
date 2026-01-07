/*
 * @Author: Detach0-0 detach0-0@outlook.com
 * @Date: 2025-12-01 19:26:00
 * @LastEditors: Detach0-0 detach0-0@outlook.com
 * @LastEditTime: 2026-01-02 19:23:43
 * @FilePath: \EGoTouchRev-vsc\HimaxChipCore\header\HimaxProtocol.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
// HimaxHal.h
#pragma once
#include <cstdint>
#include <mutex>
#include <vector>
#include <windows.h>
#include <winnt.h>

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
        
        OVERLAPPED m_ov = {};
        std::vector<uint8_t> m_xfer_buffer;
        std::recursive_mutex m_mutex;
    };

    namespace HimaxProtocol {

        bool burst_enable(HalDevice *dev, bool isEnable);
        bool register_read(HalDevice *dev, const uint8_t* addr, uint8_t* buffer, uint32_t len);
        bool register_write(HalDevice *dev, const uint8_t* addr, const uint8_t* buffer, uint32_t len);

        void build_command_packet(uint8_t cmd_id, uint8_t cmd_val, uint8_t* packet);
        bool write_and_verify(HalDevice* dev, const uint8_t* addr, const uint8_t* data, uint32_t len, uint32_t verify_len = 0);
        bool safeModeSetRaw(HalDevice* dev, const bool state);
        
        /**
         * @brief 向芯片发送命令包
         * @param dev 目标设备
         * @param cmd_id 命令 ID
         * @param cmd_val 命令值
         * @param current_slot 当前 slot（会被更新）
         * @return bool 是否成功
         */
        bool send_command(HalDevice* dev, uint8_t cmd_id, uint8_t cmd_val, uint8_t& current_slot);
    }
}