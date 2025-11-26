#pragma once
#include "HimaxPlatform.h"

namespace Himax {
    class Control {
    private:
        HalDevice* m_pMaster;
        HalDevice* m_pSlave;
        HalDevice* m_pInterrupt;
    
    public:
        Control(HalDevice* pMaster, HalDevice* pSlave, HalDevice* pInt);

        bool CheckBusReady();
        bool BurstModeEnable();
    };
}