/*
 * @Author: Detach0-0 detach0-0@outlook.com
 * @Date: 2026-01-03 01:19:27
 * @LastEditors: Detach0-0 detach0-0@outlook.com
 * @LastEditTime: 2026-01-07 13:30:34
 * @FilePath: \EGoTouchRev-vsc\HimaxChipCore\header\HimaxChip.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
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
        HX_BACK_NORMAL = 3,
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
            
            ic_operation        pic_op{};
            fw_operation        pfw_op{};
            flash_operation     pflash_op{};
            sram_operation      psram_op{};
            driver_operation    pdriver_op{};
            zf_operation        pzf_op{};
        
            
            HalDevice* SelectDevice(DeviceType type);
            bool check_bus(void);
            bool init_buffers_and_register(void);
            
            bool hx_hw_reset_ahb_intf(DeviceType type);
            bool hx_sw_reset_ahb_intf(DeviceType type); 
            bool hx_is_reload_done_ahb(void);
            bool himax_mcu_reload_disable(uint8_t disable);
            bool himax_mcu_read_FW_status(void);

            bool hx_sense_on(bool isHwReset);
            bool hx_sense_off(bool check_en);
            bool himax_mcu_power_on_init(void);
            bool switch_afe_mode(AFE_Command cmd, uint8_t param = 0);

            bool himax_mcu_assign_sorting_mode(uint8_t* tmp_data);
            bool himax_switch_data_type(DeviceType device, THP_INSPECTION_ENUM mode);
            bool himax_switch_mode_inspection(THP_INSPECTION_ENUM mode);
            void himax_mcu_interface_on(void);
            bool hx_set_N_frame(uint8_t nFrame);

            // 统一的 AFE 模式切换接口
        public:
            THP_INSPECTION_ENUM m_inspection_mode;
            std::atomic_bool isRuning;
            THP_AFE_MODE afe_mode;

            bool thp_afe_start(void);
            Chip(const std::wstring& master_path, const std::wstring& slave_path, const std::wstring& interrupt_path);
            bool IsReady(DeviceType type) const;
    };
}