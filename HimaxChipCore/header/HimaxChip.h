#pragma once
#include "HimaxHal.h"
#include <cstdint>
#include <memory>
#include <string>
#include <fstream>

namespace Himax {
std::string LogWithTimestamp(const std::string& message, const char* func = nullptr);
}

#define HIMAX_LOG(msg) Himax::LogWithTimestamp((msg), __func__)

class ic_operation {
public:
    uint8_t addr_ahb_addr_byte_0[1];				// 0x00 - AHB address byte 0
    uint8_t addr_ahb_rdata_byte_0[1];				// 0x08 - AHB read data byte 0
    uint8_t addr_ahb_access_direction[1];			// 0x0C - AHB access direction
    uint8_t addr_conti[1];							// 0x13 - Continuous mode address
    uint8_t addr_incr4[1];							// 0x0D - Increment 4 address
    uint8_t adr_i2c_psw_lb[1];						// 0x31 - I2C password lower byte
    uint8_t adr_i2c_psw_ub[1];						// 0x32 - I2C password upper byte
    uint8_t data_ahb_access_direction_read[1];		// 0x00 - AHB read access direction
    uint8_t data_conti[1];							// 0x31 - Continuous mode data
    uint8_t data_incr4[1];							// 0x10 - Increment 4 data
    uint8_t data_i2c_psw_lb[1];						// 0x27 - I2C password lower byte data
    uint8_t data_i2c_psw_ub[1];						// 0x95 - I2C password upper byte data
    uint8_t addr_tcon_on_rst[4];					// 0x80020020 - TCON on reset address
    uint8_t addr_adc_on_rst[4];						// 0x80020094 - ADC on reset address
    uint8_t addr_psl[4];							// 0x900000A0 - Power saving level address
    uint8_t addr_cs_central_state[4];				// 0x900000A8 - Central state address
    uint8_t data_rst[4];							// 0x00000000 - Reset data
    uint8_t adr_osc_en[4];							// 0x900880A8 - Oscillator enable address
    uint8_t adr_osc_pw[4];							// 0x900880E0 - Oscillator power address
};

class fw_operation {
public:
    uint8_t addr_system_reset[4];                 // 0x90000018
    uint8_t addr_ctrl_fw_isr[4];                  // 0x9000005C
    uint8_t addr_flag_reset_event[4];             // 0x900000E4
    uint8_t addr_hsen_enable[4];                  // 0x10007F14
    uint8_t addr_smwp_enable[4];                  // 0x10007F10
    uint8_t addr_program_reload_from[4];          // 0x00000000
    uint8_t addr_program_reload_to[4];            // 0x08000000
    uint8_t addr_program_reload_page_write[4];    // 0x0000FB00
    uint8_t addr_raw_out_sel[4];                  // 0x100072EC
    uint8_t addr_reload_status[4];                // 0x80050000
    uint8_t addr_reload_crc32_result[4];          // 0x80050018
    uint8_t addr_reload_addr_from[4];             // 0x80050020
    uint8_t addr_reload_addr_cmd_beat[4];         // 0x80050028
    uint8_t addr_selftest_addr_en[4];             // 0x10007F18
    uint8_t addr_criteria_addr[4];                // 0x10007F1C
    uint8_t addr_set_frame_addr[4];               // 0x10007294
    uint8_t addr_selftest_result_addr[4];         // 0x10007F24
    uint8_t addr_sorting_mode_en[4];              // 0x10007F04
    uint8_t addr_fw_mode_status[4];               // 0x10007088
    uint8_t addr_icid_addr[4];                    // 0x900000D0
    uint8_t addr_fw_ver_addr[4];                  // 0x10007004
    uint8_t addr_fw_cfg_addr[4];                  // 0x10007084
    uint8_t addr_fw_vendor_addr[4];               // 0x10007000
    uint8_t addr_cus_info[4];                     // 0x10007008
    uint8_t addr_proj_info[4];                    // 0x10007014
    uint8_t addr_fw_state_addr[4];                // 0x900000F8
    uint8_t addr_fw_dbg_msg_addr[4];              // 0x10007F40
    uint8_t addr_chk_fw_status[4];                // 0x900000A8
    uint8_t addr_dd_handshak_addr[4];             // 0x900000FC
    uint8_t addr_dd_data_addr[4];                 // 0x10007F80
    uint8_t addr_clr_fw_record_dd_sts[4];         // 0x10007FCC
    uint8_t addr_ap_notify_fw_sus[4];             // 0x10007FD0
    uint8_t data_ap_notify_fw_sus_en[4];          // 0xA55AA55A
    uint8_t data_ap_notify_fw_sus_dis[4];         // 0x00000000
    uint8_t data_system_reset[4];                 // 0x00000055
    uint8_t data_safe_mode_release_pw_active[4];  // 0x00000053
    uint8_t data_safe_mode_release_pw_reset[4];   // 0x00000000
    uint8_t data_clear[4];                        // 0x00000000
    uint8_t data_fw_stop[4];                      // 0x000000A5
    uint8_t data_program_reload_start[4];         // 0x0A3C3000
    uint8_t data_program_reload_compare[4];       // 0x04663000
    uint8_t data_program_reload_break[4];         // 0x15E75678
    uint8_t data_selftest_request[4];             // 0x00006AA6
    uint8_t data_criteria_aa_top[1];              // 0x64
    uint8_t data_criteria_aa_bot[1];              // 0x00
    uint8_t data_criteria_key_top[1];             // 0x64
    uint8_t data_criteria_key_bot[1];             // 0x00
    uint8_t data_criteria_avg_top[1];             // 0x64
    uint8_t data_criteria_avg_bot[1];             // 0x00
    uint8_t data_set_frame[4];                    // 0x0000000A
    uint8_t data_selftest_ack_hb[1];              // 0xA6
    uint8_t data_selftest_ack_lb[1];              // 0x6A
    uint8_t data_selftest_pass[1];                // 0xAA
    uint8_t data_normal_cmd[1];                   // 0x00
    uint8_t data_normal_status[1];                // 0x99
    uint8_t data_sorting_cmd[1];                  // 0xAA
    uint8_t data_sorting_status[1];               // 0xCC
    uint8_t data_dd_request[1];                   // 0xAA
    uint8_t data_dd_ack[1];                       // 0xBB
    uint8_t data_idle_dis_pwd[1];                 // 0x17
    uint8_t data_idle_en_pwd[1];                  // 0x1F
    uint8_t data_rawdata_ready_hb[1];             // 0xA3
    uint8_t data_rawdata_ready_lb[1];             // 0x3A
    uint8_t addr_ahb_addr[1];                     // 0x11
    uint8_t data_ahb_dis[1];                      // 0x00
    uint8_t data_ahb_en[1];                       // 0x01
    uint8_t addr_event_addr[1];                   // 0x30
    uint8_t addr_usb_detect[4];                   // 0x10007F38
    uint8_t addr_ulpm_33[1];                      // 0x33
    uint8_t addr_ulpm_34[1];                      // 0x34
    uint8_t data_ulpm_11[1];                      // 0x11
    uint8_t data_ulpm_22[1];                      // 0x22
    uint8_t data_ulpm_33[1];                      // 0x33
    uint8_t data_ulpm_aa[1];                      // 0xAA
    uint8_t data_normal_mode[4];                  // 0x00000000  
    uint8_t data_open_mode[4];                    // 0x00007777  
    uint8_t data_short_mode[4];                   // 0x00001111
    uint8_t data_sorting_mode[4];                 // 0x0000AAAA
};

class flash_operation {
public:
    uint8_t addr_spi200_trans_fmt[4];             // 0x80000010
    uint8_t addr_spi200_trans_ctrl[4];            // 0x80000020
    uint8_t addr_spi200_fifo_rst[4];              // 0x80000030
    uint8_t addr_spi200_rst_status[4];            // 0x80000034
    uint8_t addr_spi200_flash_speed[4];           // 0x80000040
    uint8_t addr_spi200_cmd[4];                   // 0x80000024
    uint8_t addr_spi200_addr[4];                  // 0x80000028
    uint8_t addr_spi200_data[4];                  // 0x8000002C
    uint8_t addr_spi200_bt_num[4];                // 0x800000E8

    uint8_t data_spi200_txfifo_rst[4];            // 0x00000004
    uint8_t data_spi200_rxfifo_rst[4];            // 0x00000002
    uint8_t data_spi200_trans_fmt[4];             // 0x00020780
    uint8_t data_spi200_trans_ctrl_1[4];          // 0x42000003
    uint8_t data_spi200_trans_ctrl_2[4];          // 0x47000000
    uint8_t data_spi200_trans_ctrl_3[4];          // 0x67000000
    uint8_t data_spi200_trans_ctrl_4[4];          // 0x610FF000
    uint8_t data_spi200_trans_ctrl_5[4];          // 0x694002FF
    uint8_t data_spi200_trans_ctrl_6[4];          // 0x42000000
    uint8_t data_spi200_trans_ctrl_7[4];          // 0x6940020F
    uint8_t data_spi200_cmd_1[4];                 // 0x00000005
    uint8_t data_spi200_cmd_2[4];                 // 0x00000006
    uint8_t data_spi200_cmd_3[4];                 // 0x000000C7
    uint8_t data_spi200_cmd_4[4];                 // 0x000000D8
    uint8_t data_spi200_cmd_5[4];                 // 0x00000020
    uint8_t data_spi200_cmd_6[4];                 // 0x00000002
    uint8_t data_spi200_cmd_7[4];                 // 0x0000003B
    uint8_t data_spi200_cmd_8[4];                 // 0x00000003
    uint8_t data_spi200_addr[4];                  // 0x00000000
};

class sram_operation {
public:
    uint8_t addr_mkey[4];                         // 0x100070E8
    uint8_t addr_rawdata_addr[4];                 // 0x10000000
    uint8_t addr_rawdata_end[4];                  // 0x00000000
    uint8_t passwrd[4];                           // 0x0000A55A
};

class driver_operation {
public:
    uint8_t addr_fw_define_flash_reload[4];       // 0x10007F00
    uint8_t addr_fw_define_2nd_flash_reload[4];   // 0x100072C0
    uint8_t addr_fw_define_int_is_edge[4];        // 0x10007088
    uint8_t addr_fw_define_rxnum_txnum[4];        // 0x100070F4
    uint8_t addr_fw_define_maxpt_xyrvs[4];        // 0x100070F8
    uint8_t addr_fw_define_x_y_res[4];            // 0x100070FC
    uint8_t data_df_rx[1];                        // 36
    uint8_t data_df_tx[1];                        // 18
    uint8_t data_df_pt[1];                        // 10
    uint8_t data_fw_define_flash_reload_dis[4];   // 0x0000A55A
    uint8_t data_fw_define_flash_reload_en[4];    // 0x00000000
    uint8_t data_fw_define_rxnum_txnum_maxpt_sorting[4]; // 0x00000008
    uint8_t data_fw_define_rxnum_txnum_maxpt_normal[4];  // 0x00000014
};

class zf_operation {
public:
    uint8_t data_dis_flash_reload[4];             // 0x00009AA9
    uint8_t addr_system_reset[4];                 // 0x90000018
    uint8_t data_system_reset[4];                 // 0x00000055
    uint8_t data_sram_start_addr[4];              // 0x08000000
    uint8_t data_sram_clean[4];                   // (runtime filled)
    uint8_t data_cfg_info[4];                     // 0x10007000
    uint8_t data_fw_cfg_1[4];                     // 0x10007084
    uint8_t data_fw_cfg_2[4];                     // 0x10007264
    uint8_t data_fw_cfg_3[4];                     // 0x10007300
    uint8_t data_adc_cfg_1[4];                    // 0x10006A00
    uint8_t data_adc_cfg_2[4];                    // 0x10007B28
    uint8_t data_adc_cfg_3[4];                    // 0x10007AF0
    uint8_t data_map_table[4];                    // 0x10007500
    uint8_t addr_sts_chk[4];                      // 0x900000A8
    uint8_t data_activ_sts[1];                    // 0x05
    uint8_t addr_activ_relod[4];                  // 0x90000048
    uint8_t data_activ_in[1];                     // 0xEC
};

class on_ic_operation {
public:
    uint8_t addr_ahb_addr_byte_0[1];              // 0x00
    uint8_t addr_ahb_rdata_byte_0[1];             // 0x08
    uint8_t addr_ahb_access_direction[1];         // 0x0C
    uint8_t addr_conti[1];                        // 0x13
    uint8_t addr_incr4[1];                        // 0x0D
    uint8_t adr_mcu_ctrl[1];                      // 0x82
    uint8_t data_ahb_access_direction_read[1];    // 0x00
    uint8_t data_conti[1];                        // 0x31
    uint8_t data_incr4[1];                        // 0x10
    uint8_t cmd_mcu_on[1];                        // 0x25
    uint8_t cmd_mcu_off[1];                       // 0xDA
    uint8_t adr_sleep_ctrl[1];                    // 0x99
    uint8_t cmd_sleep_in[1];                      // 0x80
    uint8_t adr_tcon_ctrl[4];                     // 0x80020000
    uint8_t cmd_tcon_on[4];                       // 0x00000000
    uint8_t adr_wdg_ctrl[4];                      // 0x9000800C
    uint8_t cmd_wdg_psw[4];                       // 0x0000AC53
    uint8_t adr_wdg_cnt_ctrl[4];                  // 0x90008010
    uint8_t cmd_wdg_cnt_clr[4];                   // 0x000035CA
};

class on_fw_operation {
public:
    uint8_t addr_smwp_enable[1];                  // 0xA2
    uint8_t addr_program_reload_from[4];          // 0x00000000
    uint8_t addr_raw_out_sel[1];                  // 0x98
    uint8_t addr_flash_checksum[4];               // 0x80000044
    uint8_t data_flash_checksum[4];               // 0x00000491
    uint8_t addr_crc_value[4];                    // 0x80000050
    uint8_t addr_reload_status[4];                // (not defined for oncell)
    uint8_t addr_reload_crc32_result[4];          // (not defined for oncell)
    uint8_t addr_reload_addr_from[4];             // (not defined for oncell)
    uint8_t addr_reload_addr_cmd_beat[4];         // (not defined for oncell)
    uint8_t addr_set_frame_addr[4];               // (reuse fw_addr_set_frame_addr if needed)
    uint8_t addr_fw_mode_status[1];               // 0x99
    uint8_t addr_icid_addr[4];                    // 0x900000D0
    uint8_t addr_fw_ver_start[1];                 // 0x90
    uint8_t data_safe_mode_release_pw_active[4];  // 0x00000053
    uint8_t data_safe_mode_release_pw_reset[4];   // 0x00000000
    uint8_t data_clear[4];                        // 0x00000000
    uint8_t addr_criteria_addr[1];                // 0x9A
    uint8_t data_selftest_pass[1];                // 0xAA
    uint8_t addr_reK_crtl[4];                     // 0x8000000C
    uint8_t data_reK_en[1];                       // 0x02
    uint8_t data_reK_dis[1];                      // 0xFD
    uint8_t data_rst_init[1];                     // 0xF0
    uint8_t data_dc_set[1];                       // 0x02
    uint8_t data_bank_set[1];                     // 0x03
    uint8_t addr_selftest_addr_en[1];             // 0x98
    uint8_t addr_selftest_result_addr[1];         // 0x9B
    uint8_t data_selftest_request[1];             // 0x06
    uint8_t data_thx_avg_mul_dc_lsb[1];           // 0x22
    uint8_t data_thx_avg_mul_dc_msb[1];           // 0x0B
    uint8_t data_thx_mul_dc_up_low_bud[1];        // 0x64
    uint8_t data_thx_avg_slf_dc_lsb[1];           // 0x14
    uint8_t data_thx_avg_slf_dc_msb[1];           // 0x05
    uint8_t data_thx_slf_dc_up_low_bud[1];        // 0x64
    uint8_t data_thx_slf_bank_up[1];              // 0x40
    uint8_t data_thx_slf_bank_low[1];             // 0x00
    uint8_t data_idle_dis_pwd[1];                 // 0x40
    uint8_t data_idle_en_pwd[1];                  // 0x00
    uint8_t data_rawdata_ready_hb[1];             // 0xA3
    uint8_t data_rawdata_ready_lb[1];             // 0x3A
    uint8_t addr_ahb_addr[1];                     // 0x11
    uint8_t data_ahb_dis[1];                      // 0x00
    uint8_t data_ahb_en[1];                       // 0x01
    uint8_t addr_event_addr[1];                   // 0x30
    uint8_t addr_usb_detect[1];                   // 0xA4
};

class on_flash_operation {
public:
    uint8_t addr_ctrl_base[4];                    // 0x80000000
    uint8_t addr_ctrl_auto[4];                    // 0x80000004
    uint8_t data_main_erase[4];                   // 0x0000A50D
    uint8_t data_auto[1];                         // 0xA5
    uint8_t data_main_read[1];                    // 0x03
    uint8_t data_page_write[1];                   // 0x05
    uint8_t data_sfr_read[1];                     // 0x14
    uint8_t data_spp_read[1];                     // 0x10
    uint8_t addr_ahb_ctrl[4];                     // 0x80000020
    uint8_t data_ahb_squit[4];                    // 0x00000001

    uint8_t addr_unlock_0[4];                     // 0x00000000
    uint8_t addr_unlock_4[4];                     // 0x00000004
    uint8_t addr_unlock_8[4];                     // 0x00000008
    uint8_t addr_unlock_c[4];                     // 0x0000000C
    uint8_t data_cmd0[4];                         // 0x28178EA0
    uint8_t data_cmd1[4];                         // 0x0A0E03FF
    uint8_t data_cmd2[4];                         // 0x8C203D0C
    uint8_t data_cmd3[4];                         // 0x00300263
    uint8_t data_lock[4];                         // 0x03400000
};

class on_sram_operation {
public:
    uint8_t addr_rawdata_addr[4];                 // 0x10000400
    uint8_t addr_rawdata_end[4];                  // 0x00000000
    uint8_t data_conti[4];                        // 0x44332211
    uint8_t data_fin[4];                          // 0x00000000
    uint8_t passwrd[4];                           // 0x00005AA5
};

class on_driver_operation {
public:
    uint8_t addr_fw_define_int_is_edge[4];        // 0x10007088
    uint8_t addr_fw_rx_tx_maxpt_num[4];           // 0x08000004 or 0x0800001C (depends on HX_NEW_EVENT_STACK_FORMAT)
#if defined(HX_NEW_EVENT_STACK_FORMAT)
    uint8_t addr_fw_maxpt_bt_num[4];              // 0x0800000C
#endif
    uint8_t addr_fw_xy_rev_int_edge[4];           // 0x08000110 or 0x0800000C
    uint8_t addr_fw_define_x_y_res[4];            // 0x08000010 or 0x08000030
    uint8_t data_fw_define_rxnum_txnum_maxpt_sorting[4]; // 0x00000008
    uint8_t data_fw_define_rxnum_txnum_maxpt_normal[4];  // 0x00000014
    uint8_t data_df_rx[1];                        // 28
    uint8_t data_df_tx[1];                        // 14
    uint8_t data_df_pt[1];                        // 10
};

namespace Himax {

class Chip {
private:
    std::unique_ptr<HalDevice> m_master;
    std::unique_ptr<HalDevice> m_slave;
    std::unique_ptr<HalDevice> m_interrupt;

    uint32_t hx_mode;

    ic_operation        m_ic_op{};
    fw_operation        m_fw_op{};
    flash_operation     m_flash_op{};
    sram_operation      m_sram_op{};
    driver_operation    m_driver_op{};
    zf_operation        m_zf_op{};
    on_ic_operation     m_on_ic_op{};
    on_fw_operation     m_on_fw_op{};
    on_flash_operation  m_on_flash_op{};
    on_sram_operation   m_on_sram_op{};
    on_driver_operation m_on_driver_op{};
    
    HalDevice* SelectDevice(DeviceType type);

    bool check_bus(void);
    
    bool hx_reload_set(uint8_t state);
    bool init_buffers_and_register(void);
    
    bool hx_hw_reset_ahb_intf(DeviceType type);
    bool hx_sw_reset_ahb_intf(DeviceType type); 
    bool hx_is_reload_done_ahb(void);

    bool hx_set_N_frame(uint8_t nFrame);
    bool hx_set_raw_data_type(uint32_t hx_mode, bool state);
    bool hx_switch_mode(uint32_t mode);
    bool hx_sense_on(bool isHwReset);

    void thp_afe_stop(void);
    void InitLogFile();
    
    std::ofstream m_logFile;
    std::string message;
    public:
    void thp_afe_start(void);
    Chip(const std::wstring& master_path, const std::wstring& slave_path, const std::wstring& interrupt_path);
    bool IsReady(DeviceType type) const;

    bool WaitInterrupt();
    bool GetMasterData(std::vector<uint8_t>& buffer);
    bool GetSlaveData(std::vector<uint8_t>& buffer);
};
}