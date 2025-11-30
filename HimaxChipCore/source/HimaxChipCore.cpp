#include "HimaxChipCore.h"
#include "HimaxPlatform.h"
#include <cstdint>
#include <print>

namespace Himax {

    static const uint8_t REG_AHB_ADDR_BYTE_0[] = {0x00}; // 用于设置读取地址
    static const uint8_t REG_AHB_RDATA_BYTE_0[] = {0x08}; // 用于读取数据
    static const uint8_t REG_PASSWORD[]  = {0x31};
    static const uint8_t REG_BURST_EN[]  = {0x13};
    static const uint8_t REG_AUTO_INC[]  = {0x0D};
    static const uint8_t REG_ADDR_INC[]  = {0x11};

    static const uint8_t REG_DEBUG_A8[]  = {0xA8, 0x00, 0x00, 0x90}; // FW Status
    static const uint8_t REG_DEBUG_E4[]  = {0xE4, 0x00, 0x00, 0x90}; // Reset Event
    static const uint8_t VAL_UNLOCK[]    = {0x27, 0x95};
    static const uint8_t VAL_LOCK[]      = {0x00, 0x00};

    Control::Control(HalDevice* pMaster, HalDevice* pSlave, HalDevice* pInt)
        : m_pMaster(pMaster), m_pSlave(pSlave), m_pInterrupt(pInt) {

    }

    bool Control::CheckBusReady() {
        uint8_t tmp[1] = {0};

        m_pSlave->ReadRegister(0x13, tmp, 1);
        if (tmp[0] == 0x00) {    
            std::print("Slave bus read failed!\n");
            return false;
        }
        std::print("Slave bus check success!: 0x\n");

        m_pMaster->ReadRegister(0x13, tmp, 1);
        if (tmp[0] == 0x00) {
            std::print("Master bus check failed!\n");
            return false;
        }
        std::print("Master bus check success!: 0x\n");

        return true;
    }

/*     bool Control::addrIncUnitSetRaw_int(void) {
        return m_pMaster->WriteRegister(REG_ADDR_INC, 1, (uint8_t)0x01, 1);
    }
    
    bool Control::BurstModeEnable(int sign) {
        return m_pMaster->WriteRegister(REG_BURST_EN, (uint8_t)0x31);
    }

    bool Control::wrAddrAutoincSetRaw_intf(int sign) {
        uint8_t data = (sign == 0) ? 0x12 : 0x13;
        return m_pMaster->WriteRegister(REG_AUTO_INC, data);
    } */

    bool Control::safeModeSetRaw_intf(int enable, uint8_t DevID) {
        // 1. 选择设备
        HalDevice* targetDev = nullptr;
        if (DevID == 0) targetDev = m_pMaster;
        else if (DevID == 1) targetDev = m_pSlave;
        else return false;
        
        uint8_t writeVal[2];
        if (enable) {
            std::copy(std::begin(VAL_UNLOCK), std::end(VAL_UNLOCK), writeVal);
        } else {
            std::copy(std::begin(VAL_LOCK), std::end(VAL_LOCK), writeVal);
        }
        uint8_t readVal[2] = {0x00, 0x00};

        // 2. 写入密码 (0x31)
        // 调用 WriteRegister(uint32_t addr, uint16_t val) 重载
        // 自动处理 F2 31 27 95
        if (!targetDev->WriteRegister(REG_PASSWORD, 1, writeVal, 2)) {
            return false;
        }

        // 3. 回读校验 (0x31)
        // 这里的 ReadRegister 是 HalDevice 的方法，读取 2 字节
        if (!targetDev->ReadRegister(REG_PASSWORD[0], readVal, 2)) {
            return false;
        }

        // 4. 比较
        if (!std::equal(std::begin(readVal), std::end(readVal), std::begin(writeVal))) {
            // 校验失败，读取调试信息 (A8 和 E4)
            // 这是一个“间接读取”过程：先写地址到 0x00，再从 0x08 读数据
            uint32_t valA8 = 0; 
            uint32_t valE4 = 0;

            // 读取 A8 (32位地址)
            // 步骤 A: 向 0x00 写入要读取的地址 (利用 32位写入重载)
            targetDev->WriteRegister(REG_AHB_ADDR_BYTE_0, 1, REG_DEBUG_A8, 4);
            // 步骤 B: 从 0x08 读取 4 字节数据
            targetDev->ReadRegister(REG_AHB_RDATA_BYTE_0[0], (uint8_t*)&valA8, 4);

            // 读取 E4
            targetDev->WriteRegister(REG_AHB_ADDR_BYTE_0, 1, REG_DEBUG_E4, 4);
            targetDev->ReadRegister(REG_AHB_RDATA_BYTE_0[0], (uint8_t*)&valE4, 4);
            
            // 打印错误日志
            return false;
        }

        return true;
    }
}
