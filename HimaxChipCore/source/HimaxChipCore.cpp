#include "HimaxChipCore.h"
#include <cstdint>
#include <print>

namespace Himax {
    Control::Control(HalDevice* pMaster, HalDevice* pSlave, HalDevice* pInt)
        : m_pMaster(pMaster), m_pSlave(pSlave), m_pInterrupt(pInt) {

    }

    bool Control::CheckBusReady() {
        uint8_t tmp[1] = {0};

        m_pMaster->ReadRegister(0x13, tmp, 1);
        if (tmp[0] != 0x31) {
            std::print("Master bus check failed!\n");
            return false;
        }
        std::print("Master bus check success!: 0x%x\n", tmp[0]);
        m_pSlave->ReadRegister(0x13, tmp, 1);
        if (tmp[0] != 0x31) {    
            std::print("Slave bus read failed!\n");
            return false;
        }
        std::print("Slave bus check success!: 0x%x\n", tmp[0]);
        return true;
    }
}