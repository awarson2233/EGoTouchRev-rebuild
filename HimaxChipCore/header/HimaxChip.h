#pragma once
#include "HimaxHal.h"
#include "HimaxCommon.h"
#include "HimaxLogger.h"
#include "HimaxRegisters.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <array>

namespace Himax {

    enum class THP_AFE_MODE {
        HX_RAWDATA = 0,
        HX_ACT_IDLE_RAWDATA = 1,
        HX_LP_IDLE_RAWDATA = 2,
    };

    enum class THP_AFE_STATUS {
        FATAL_ERROR = -1,
        STOP = 0,
        BUS_FAIL = 1,
        INITIALZING = 2,
        CHANGE = 3,
        RUNNING = 4,
    };

    
    class Chip {
        private:
            std::unique_ptr<HalDevice> m_master;
            std::unique_ptr<HalDevice> m_slave;
            std::unique_ptr<HalDevice> m_interrupt;

            uint32_t hx_mode;
            uint32_t addr_unknown;
            uint8_t current_slot;
            std::string message;

            // Register definitions - kept as members to minimize code changes in logic
            ic_operation        m_ic_op;
            fw_operation        m_fw_op;
            flash_operation     m_flash_op;
            sram_operation      m_sram_op;
            driver_operation    m_driver_op;
            zf_operation        m_zf_op;
            on_ic_operation     m_on_ic_op;
            on_fw_operation     m_on_fw_op;
            on_flash_operation  m_on_flash_op;
            on_sram_operation   m_on_sram_op;
            on_driver_operation m_on_driver_op;
            
            HalDevice* SelectDevice(DeviceType type);

            bool check_bus(void);
            
            bool hx_reload_set(uint8_t state);
            bool init_buffers_and_register(void);

            bool hx_send_command(uint8_t param_1, uint8_t param_3);
            bool hx_hw_reset_ahb_intf(DeviceType type);
            bool hx_sw_reset_ahb_intf(DeviceType type); 
            bool hx_is_reload_done_ahb(void);

            bool hx_set_N_frame(uint8_t nFrame);
            bool hx_set_raw_data_type(DeviceType type, uint32_t hx_mode);
            ChipResult<void> hx_switch_mode(uint32_t mode);

            ChipResult<void> hx_sense_on(bool isHwReset);
            bool hx_sense_off(bool check_en);

            bool thp_afe_clear_status(uint8_t param_1);
            void thp_afe_stop(void);
            
            bool thp_afe_enable_freq_shift();
            bool hx_ts_work(std::array<uint8_t, 5063> master_frame, std::array<uint8_t, 339> slave_frame);

            // Hook for automatic mode switching based on frame analysis
            void ProcessAutoModeSwitch(const std::array<uint8_t, 5063>& frame);

        public:
            THP_AFE_STATUS m_status;
            THP_AFE_MODE m_mode;
            std::atomic<bool> m_system_run_flag{true};
            
            void thp_afe_start(void);
            Chip(const std::wstring& master_path, const std::wstring& slave_path, const std::wstring& interrupt_path);
            bool IsReady(DeviceType type) const;

            void SetSystemRunFlag(bool run) { m_system_run_flag = run; }
    };
}
