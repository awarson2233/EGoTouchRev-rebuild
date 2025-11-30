#pragma once
#include "HimaxPlatform.h"
#include <cstdint>
#include <minwindef.h>
#include <winnt.h>

namespace Himax {
    class Control {
    private:
        HalDevice* m_pMaster;
        HalDevice* m_pSlave;
        HalDevice* m_pInterrupt;
    
    public:
        Control(HalDevice* pMaster, HalDevice *pSlave, HalDevice* pInt);

        bool CheckBusReady(void);
        bool addrIncUnitSetRaw_int(void);
        bool BurstModeEnable(int sign);
        bool wrAddrAutoincSetRaw_intf(int sign);
        bool safeModeSetRaw_intf(int enable, uint8_t DevID);
    };
}