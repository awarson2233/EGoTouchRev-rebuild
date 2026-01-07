#pragma once
#include "HimaxProtocol.h"
#include "HimaxRegisters.h"
#include "Logger.h"
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

// 使用 Common::Logger 进行日志记录
#define HIMAX_LOG(msg) m_log.Log((msg), __func__)

namespace Himax {

    enum class THP_INSPECTION_ENUM {
        HX_RAWDATA = 0,
        HX_ACT_IDLE_RAWDATA = 1,
        HX_LP_IDLE_RAWDATA = 2,
    };

    enum class THP_AFE_MODE {
        Stop = 0,
        Normal = 1,
        Idle = 2,
        Turbo = 3,
    };

    // AFE 命令类型
    enum class AFE_Command : uint8_t {
        ClearStatus = 0,       // 清除状态
        EnableFreqShift,       // 启用频移
        DisableFreqShift,      // 禁用频移
        StartCalibration,      // 开始校准
        EnterIdle,             // 进入空闲
    };
    
    class Chip {
        private:
            std::unique_ptr<HalDevice> m_master;
            std::unique_ptr<HalDevice> m_slave;
            std::unique_ptr<HalDevice> m_interrupt;

            Common::Logger m_log{"HIMAX"};
            
            uint8_t current_slot;
            
            ic_operation        m_ic_op{};
            fw_operation        m_fw_op{};
            flash_operation     m_flash_op{};
            sram_operation      m_sram_op{};
            driver_operation    m_driver_op{};
            zf_operation        m_zf_op{};
        
            
            HalDevice* SelectDevice(DeviceType type);
            bool check_bus(void);
            
            bool hx_hw_reset_ahb_intf(DeviceType type);
            bool hx_sw_reset_ahb_intf(DeviceType type); 
            bool hx_is_reload_done_ahb(void);
            bool hx_reload_set(uint8_t state);
            
            bool init_buffers_and_register(void);
            bool hx_set_N_frame(uint8_t nFrame);
            bool hx_set_raw_data_type(DeviceType device, THP_INSPECTION_ENUM mode);
            bool himax_switch_mode_inspection(THP_INSPECTION_ENUM mode);
            
            bool hx_sense_on(bool isHwReset);
            bool hx_sense_off(bool check_en);
            
            // 统一的 AFE 模式切换接口
            bool switch_afe_mode(AFE_Command cmd, uint8_t param = 0);
        public:
            THP_INSPECTION_ENUM m_inspection_mode;
            std::atomic_bool isRuning;
            THP_AFE_MODE afe_mode;
            bool TouchEngine();

            bool thp_afe_start(void);
            Chip(const std::wstring& master_path, const std::wstring& slave_path, const std::wstring& interrupt_path);
            bool IsReady(DeviceType type) const;
    };
}