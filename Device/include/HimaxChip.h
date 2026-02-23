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
#include <atomic>
#include <cstdint>
#include <memory>
#include <array>
#include <string>
#include <winnt.h>

// Note: The legacy HIMAX_LOG is removed. We use LOG_INFO, LOG_ERROR, etc. from Logger.h

namespace Himax {

    enum class ConnectionState {
        Unconnected = 0,    // 未连接/句柄未打开
        Connected = 1,      // 通信就绪
        Error = 2           // 通信断开错误
    };

    enum class THP_INSPECTION_ENUM {
        HX_RAWDATA = 0,
        HX_ACT_IDLE_RAWDATA = 1,
        HX_LP_IDLE_RAWDATA = 2,
        HX_BACK_NORMAL = 3,
        EGO_RAWDATA = 4,
    };
    
    enum class THP_AFE_MODE {
        Stop = 0,
        Normal = 2,
        Idle = 3,
        Turbo = 4,
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
            
            uint8_t current_slot;
            
            ic_operation        pic_op{};
            fw_operation        pfw_op{};
            flash_operation     pflash_op{};
            sram_operation      psram_op{};
            driver_operation    pdriver_op{};
            zf_operation        pzf_op{};
        
            
            ChipResult<HalDevice*> SelectDevice(DeviceType type);
            ChipResult<> check_bus(void);
            ChipResult<> init_buffers_and_register(void);
            
            ChipResult<> hx_hw_reset_ahb_intf(DeviceType type);
            ChipResult<> hx_sw_reset_ahb_intf(DeviceType type); 
            ChipResult<> hx_is_reload_done_ahb(void);
            ChipResult<> himax_mcu_reload_disable(uint8_t disable);
            ChipResult<> himax_mcu_read_FW_status(void);

            ChipResult<> hx_sense_on(bool isHwReset);
            ChipResult<> hx_sense_off(bool check_en);
            ChipResult<> himax_mcu_power_on_init(void);
            ChipResult<> switch_afe_mode(AFE_Command cmd, uint8_t param = 0);

            ChipResult<> himax_mcu_assign_sorting_mode(uint8_t* tmp_data);
            ChipResult<> himax_switch_data_type(DeviceType device, THP_INSPECTION_ENUM mode);
            ChipResult<> himax_switch_mode_inspection(THP_INSPECTION_ENUM mode);
            ChipResult<> himax_mcu_interface_on(void);
            ChipResult<> hx_set_N_frame(uint8_t nFrame);

            // 统一的 AFE 模式切换接口
        public:
            alignas(64) std::array<uint8_t, 6000> back_data{};
            THP_INSPECTION_ENUM m_inspection_mode;
            THP_AFE_MODE afe_mode;
            
            // 主机的连接状态机
            std::atomic<ConnectionState> m_connState{ConnectionState::Unconnected};

            // 用于日志的前缀转换辅助函数
            const char* GetStateStr() const {
                switch(m_connState.load()) {
                    case ConnectionState::Unconnected: return "Unconnected";
                    case ConnectionState::Connected: return "Connected";
                    case ConnectionState::Error: return "Error";
                    default: return "Unknown";
                }
            }


            ChipResult<> thp_afe_enter_idle(uint8_t param = 0);
            ChipResult<> thp_afe_force_exit_idle(void);
            ChipResult<> thp_afe_start_calibration(uint8_t param = 0);
            ChipResult<> thp_afe_enable_freq_shift(void);
            ChipResult<> thp_afe_disable_freq_shift(void);
            ChipResult<> thp_afe_clear_status(uint8_t cmd_val);
            ChipResult<> thp_afe_force_to_freq_point(uint8_t freq_idx);
            ChipResult<> thp_afe_force_to_scan_rate(uint8_t rate_idx);

            Chip(const std::wstring& master_path, const std::wstring& slave_path, const std::wstring& interrupt_path);
            ~Chip(); // Add destructor for explicit cleanup
            
            bool IsReady(DeviceType type) const;
            ConnectionState GetConnectionState() const { return m_connState.load(); }
            
            ChipResult<> Init(void);
            ChipResult<> Deinit(void); // Replaces Stop
            
            ChipResult<> GetFrame(void);
    };
}