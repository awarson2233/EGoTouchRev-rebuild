#include "HimaxChip.h"
#include "HimaxHal.h"
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <string>
#include <synchapi.h>
#include <vector>
#include <wingdi.h>

namespace {
constexpr uint8_t ByteAt(uint32_t value, uint8_t index) noexcept {
    return static_cast<uint8_t>((value >> (index * 8)) & 0xFFu);
}

inline void WriteU32(uint8_t (&dest)[4], uint32_t value) noexcept {
    dest[0] = ByteAt(value, 0);
    dest[1] = ByteAt(value, 1);
    dest[2] = ByteAt(value, 2);
    dest[3] = ByteAt(value, 3);
}

inline void WriteU16(uint8_t (&dest)[2], uint16_t value) noexcept {
    dest[0] = static_cast<uint8_t>(value & 0xFFu);
    dest[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
}

std::ofstream* g_log_file = nullptr;
}

namespace Himax {

std::string LogWithTimestamp(const std::string& message, const char* func) {
    using namespace std::chrono;

    const auto now = system_clock::now();
    const auto sec_tp = floor<seconds>(now);
    const auto usec = duration_cast<microseconds>(now - sec_tp).count();

    const std::string line = std::format(
        "HIMAX [{:%H:%M:%S}:{:06d}] [{}] {}",
        sec_tp,
        static_cast<int>(usec),
        func ? func : "-",
        message);

    std::cout << line << std::endl;
    if (g_log_file && g_log_file->is_open()) {
        (*g_log_file) << line << std::endl;
    }
    return line;
}

namespace {
ic_operation InitIcOperation();
fw_operation InitFwOperation();
flash_operation InitFlashOperation();
sram_operation InitSramOperation();
driver_operation InitDriverOperation();
zf_operation InitZfOperation();
on_ic_operation InitOnIcOperation();
on_fw_operation InitOnFwOperation();
on_flash_operation InitOnFlashOperation();
on_sram_operation InitOnSramOperation();
on_driver_operation InitOnDriverOperation();
}

Chip::Chip(const std::wstring& master_path, const std::wstring& slave_path, const std::wstring& interrupt_path)
    : m_ic_op(InitIcOperation()),
      m_fw_op(InitFwOperation()),
      m_flash_op(InitFlashOperation()),
      m_sram_op(InitSramOperation()),
      m_driver_op(InitDriverOperation()),
      m_zf_op(InitZfOperation()),
      m_on_ic_op(InitOnIcOperation()),
      m_on_fw_op(InitOnFwOperation()),
      m_on_flash_op(InitOnFlashOperation()),
      m_on_sram_op(InitOnSramOperation()),
      m_on_driver_op(InitOnDriverOperation())
{
    m_master = std::make_unique<HalDevice>(master_path.c_str(), DeviceType::Master);
    m_slave  = std::make_unique<HalDevice>(slave_path.c_str(), DeviceType::Slave);
    m_interrupt = std::make_unique<HalDevice>(interrupt_path.c_str(), DeviceType::Interrupt);
    
    hx_mode = 0x105;
    addr_unknown = 0x00;

    InitLogFile();
}

void Chip::InitLogFile() {
    namespace fs = std::filesystem;
    using namespace std::chrono;

    const auto today = floor<days>(system_clock::now());
    const std::string date_str = std::format("{:%Y%m%d}", today);

    const fs::path dir_path{"C:/ProgramData/EGoTouchRev/"};
    std::error_code ec;
    fs::create_directories(dir_path, ec);

    const fs::path file_path = dir_path / std::format("hx_hal_log_{}", date_str);
    const bool existed = fs::exists(file_path);

    m_logFile.open(file_path, std::ios::out | std::ios::app);
    if (m_logFile.is_open()) {
        g_log_file = &m_logFile;
        if (existed) {
            const std::string sep(40, '-');
            m_logFile << sep << '\n'
                      << '\n' << '\n' << '\n' << '\n' << '\n'
                      << sep << '\n';
            m_logFile.flush();
        }
    }
}

namespace {
ic_operation InitIcOperation() {
    ic_operation op{};
    op.addr_ahb_addr_byte_0[0] = 0x00;
    op.addr_ahb_rdata_byte_0[0] = 0x08;
    op.addr_ahb_access_direction[0] = 0x0C;
    op.addr_conti[0] = 0x13;
    op.addr_incr4[0] = 0x0D;
    op.adr_i2c_psw_lb[0] = 0x31;
    op.adr_i2c_psw_ub[0] = 0x32;
    op.data_ahb_access_direction_read[0] = 0x00;
    op.data_conti[0] = 0x31;
    op.data_incr4[0] = 0x10;
    op.data_i2c_psw_lb[0] = 0x27;
    op.data_i2c_psw_ub[0] = 0x95;
    WriteU32(op.addr_tcon_on_rst, 0x80020020);
    WriteU32(op.addr_adc_on_rst, 0x80020094);
    WriteU32(op.addr_psl, 0x900000A0);
    WriteU32(op.addr_cs_central_state, 0x900000A8);
    WriteU32(op.data_rst, 0x00000000);
    WriteU32(op.adr_osc_en, 0x900880A8);
    WriteU32(op.adr_osc_pw, 0x900880E0);
    return op;
}

fw_operation InitFwOperation() {
    fw_operation op{};
    WriteU32(op.addr_system_reset, 0x90000018);
    WriteU32(op.addr_ctrl_fw_isr, 0x9000005C);
    WriteU32(op.addr_flag_reset_event, 0x900000E4);
    WriteU32(op.addr_hsen_enable, 0x10007F14);
    WriteU32(op.addr_smwp_enable, 0x10007F10);
    WriteU32(op.addr_program_reload_from, 0x00000000);
    WriteU32(op.addr_program_reload_to, 0x08000000);
    WriteU32(op.addr_program_reload_page_write, 0x0000FB00);
    WriteU32(op.addr_raw_out_sel, 0x100072EC);
    WriteU32(op.addr_reload_status, 0x80050000);
    WriteU32(op.addr_reload_crc32_result, 0x80050018);
    WriteU32(op.addr_reload_addr_from, 0x80050020);
    WriteU32(op.addr_reload_addr_cmd_beat, 0x80050028);
    WriteU32(op.addr_selftest_addr_en, 0x10007F18);
    WriteU32(op.addr_criteria_addr, 0x10007F1C);
    WriteU32(op.addr_set_frame_addr, 0x10007294);
    WriteU32(op.addr_selftest_result_addr, 0x10007F24);
    WriteU32(op.addr_sorting_mode_en, 0x10007F04);
    WriteU32(op.addr_fw_mode_status, 0x10007088);
    WriteU32(op.addr_icid_addr, 0x900000D0);
    WriteU32(op.addr_fw_ver_addr, 0x10007004);
    WriteU32(op.addr_fw_cfg_addr, 0x10007084);
    WriteU32(op.addr_fw_vendor_addr, 0x10007000);
    WriteU32(op.addr_cus_info, 0x10007008);
    WriteU32(op.addr_proj_info, 0x10007014);
    WriteU32(op.addr_fw_state_addr, 0x900000F8);
    WriteU32(op.addr_fw_dbg_msg_addr, 0x10007F40);
    WriteU32(op.addr_chk_fw_status, 0x900000A8);
    WriteU32(op.addr_dd_handshak_addr, 0x900000FC);
    WriteU32(op.addr_dd_data_addr, 0x10007F80);
    WriteU32(op.addr_clr_fw_record_dd_sts, 0x10007FCC);
    WriteU32(op.addr_ap_notify_fw_sus, 0x10007FD0);
    WriteU32(op.data_ap_notify_fw_sus_en, 0xA55AA55A);
    WriteU32(op.data_ap_notify_fw_sus_dis, 0x00000000);
    WriteU32(op.data_system_reset, 0x00000055);
    WriteU32(op.data_safe_mode_release_pw_active, 0x00000053);
    WriteU32(op.data_safe_mode_release_pw_reset, 0x00000000);
    WriteU32(op.data_clear, 0x00000000);
    WriteU32(op.data_fw_stop, 0x000000A5);
    WriteU32(op.data_program_reload_start, 0x0A3C3000);
    WriteU32(op.data_program_reload_compare, 0x04663000);
    WriteU32(op.data_program_reload_break, 0x15E75678);
    WriteU32(op.data_selftest_request, 0x00006AA6);
    op.data_criteria_aa_top[0] = 0x64;
    op.data_criteria_aa_bot[0] = 0x00;
    op.data_criteria_key_top[0] = 0x64;
    op.data_criteria_key_bot[0] = 0x00;
    op.data_criteria_avg_top[0] = 0x64;
    op.data_criteria_avg_bot[0] = 0x00;
    WriteU32(op.data_set_frame, 0x0000000A);
    op.data_selftest_ack_hb[0] = 0xA6;
    op.data_selftest_ack_lb[0] = 0x6A;
    op.data_selftest_pass[0] = 0xAA;
    op.data_normal_cmd[0] = 0x00;
    op.data_normal_status[0] = 0x99;
    op.data_sorting_cmd[0] = 0xAA;
    op.data_sorting_status[0] = 0xCC;
    op.data_dd_request[0] = 0xAA;
    op.data_dd_ack[0] = 0xBB;
    op.data_idle_dis_pwd[0] = 0x17;
    op.data_idle_en_pwd[0] = 0x1F;
    op.data_rawdata_ready_hb[0] = 0xA3;
    op.data_rawdata_ready_lb[0] = 0x3A;
    op.addr_ahb_addr[0] = 0x11;
    op.data_ahb_dis[0] = 0x00;
    op.data_ahb_en[0] = 0x01;
    op.addr_event_addr[0] = 0x30;
    WriteU32(op.addr_usb_detect, 0x10007F38);
    op.addr_ulpm_33[0] = 0x33;
    op.addr_ulpm_34[0] = 0x34;
    op.data_ulpm_11[0] = 0x11;
    op.data_ulpm_22[0] = 0x22;
    op.data_ulpm_33[0] = 0x33;
    op.data_ulpm_aa[0] = 0xAA;
    WriteU32(op.data_normal_mode, 0x00000000);
    WriteU32(op.data_open_mode, 0x00007777);
    WriteU32(op.data_short_mode, 0x00001111);
    WriteU32(op.data_sorting_mode, 0x0000AAAA);
    return op;
}

flash_operation InitFlashOperation() {
    flash_operation op{};
    WriteU32(op.addr_spi200_trans_fmt, 0x80000010);
    WriteU32(op.addr_spi200_trans_ctrl, 0x80000020);
    WriteU32(op.addr_spi200_fifo_rst, 0x80000030);
    WriteU32(op.addr_spi200_rst_status, 0x80000034);
    WriteU32(op.addr_spi200_flash_speed, 0x80000040);
    WriteU32(op.addr_spi200_cmd, 0x80000024);
    WriteU32(op.addr_spi200_addr, 0x80000028);
    WriteU32(op.addr_spi200_data, 0x8000002C);
    WriteU32(op.addr_spi200_bt_num, 0x800000E8);
    WriteU32(op.data_spi200_txfifo_rst, 0x00000004);
    WriteU32(op.data_spi200_rxfifo_rst, 0x00000002);
    WriteU32(op.data_spi200_trans_fmt, 0x00020780);
    WriteU32(op.data_spi200_trans_ctrl_1, 0x42000003);
    WriteU32(op.data_spi200_trans_ctrl_2, 0x47000000);
    WriteU32(op.data_spi200_trans_ctrl_3, 0x67000000);
    WriteU32(op.data_spi200_trans_ctrl_4, 0x610FF000);
    WriteU32(op.data_spi200_trans_ctrl_5, 0x694002FF);
    WriteU32(op.data_spi200_trans_ctrl_6, 0x42000000);
    WriteU32(op.data_spi200_trans_ctrl_7, 0x6940020F);
    WriteU32(op.data_spi200_cmd_1, 0x00000005);
    WriteU32(op.data_spi200_cmd_2, 0x00000006);
    WriteU32(op.data_spi200_cmd_3, 0x000000C7);
    WriteU32(op.data_spi200_cmd_4, 0x000000D8);
    WriteU32(op.data_spi200_cmd_5, 0x00000020);
    WriteU32(op.data_spi200_cmd_6, 0x00000002);
    WriteU32(op.data_spi200_cmd_7, 0x0000003B);
    WriteU32(op.data_spi200_cmd_8, 0x00000003);
    WriteU32(op.data_spi200_addr, 0x00000000);
    return op;
}

sram_operation InitSramOperation() {
    sram_operation op{};
    WriteU32(op.addr_mkey, 0x100070E8);
    WriteU32(op.addr_rawdata_addr, 0x10000000);
    WriteU32(op.addr_rawdata_end, 0x00000000);
    WriteU32(op.passwrd, 0x0000A55A);
    return op;
}

driver_operation InitDriverOperation() {
    driver_operation op{};
    WriteU32(op.addr_fw_define_flash_reload, 0x10007F00);
    WriteU32(op.addr_fw_define_2nd_flash_reload, 0x100072C0);
    WriteU32(op.addr_fw_define_int_is_edge, 0x10007088);
    WriteU32(op.addr_fw_define_rxnum_txnum, 0x100070F4);
    WriteU32(op.addr_fw_define_maxpt_xyrvs, 0x100070F8);
    WriteU32(op.addr_fw_define_x_y_res, 0x100070FC);
    op.data_df_rx[0] = 0x24;
    op.data_df_tx[0] = 0x12;
    op.data_df_pt[0] = 0x0A;
    WriteU32(op.data_fw_define_flash_reload_dis, 0x0000A55A);
    WriteU32(op.data_fw_define_flash_reload_en, 0x00000000);
    WriteU32(op.data_fw_define_rxnum_txnum_maxpt_sorting, 0x00000008);
    WriteU32(op.data_fw_define_rxnum_txnum_maxpt_normal, 0x00000014);
    return op;
}

zf_operation InitZfOperation() {
    zf_operation op{};
    WriteU32(op.data_dis_flash_reload, 0x00009AA9);
    WriteU32(op.addr_system_reset, 0x90000018);
    WriteU32(op.data_system_reset, 0x00000055);
    WriteU32(op.data_sram_start_addr, 0x08000000);
    WriteU32(op.data_cfg_info, 0x10007000);
    WriteU32(op.data_fw_cfg_1, 0x10007084);
    WriteU32(op.data_fw_cfg_2, 0x10007264);
    WriteU32(op.data_fw_cfg_3, 0x10007300);
    WriteU32(op.data_adc_cfg_1, 0x10006A00);
    WriteU32(op.data_adc_cfg_2, 0x10007B28);
    WriteU32(op.data_adc_cfg_3, 0x10007AF0);
    WriteU32(op.data_map_table, 0x10007500);
    WriteU32(op.addr_sts_chk, 0x900000A8);
    op.data_activ_sts[0] = 0x05;
    WriteU32(op.addr_activ_relod, 0x90000048);
    op.data_activ_in[0] = 0xEC;
    return op;
}

on_ic_operation InitOnIcOperation() {
    on_ic_operation op{};
    op.addr_ahb_addr_byte_0[0] = 0x00;
    op.addr_ahb_rdata_byte_0[0] = 0x08;
    op.addr_ahb_access_direction[0] = 0x0C;
    op.addr_conti[0] = 0x13;
    op.addr_incr4[0] = 0x0D;
    op.adr_mcu_ctrl[0] = 0x82;
    op.data_ahb_access_direction_read[0] = 0x00;
    op.data_conti[0] = 0x31;
    op.data_incr4[0] = 0x10;
    op.cmd_mcu_on[0] = 0x25;
    op.cmd_mcu_off[0] = 0xDA;
    op.adr_sleep_ctrl[0] = 0x99;
    op.cmd_sleep_in[0] = 0x80;
    WriteU32(op.adr_tcon_ctrl, 0x80020000);
    WriteU32(op.cmd_tcon_on, 0x00000000);
    WriteU32(op.adr_wdg_ctrl, 0x9000800C);
    WriteU32(op.cmd_wdg_psw, 0x0000AC53);
    WriteU32(op.adr_wdg_cnt_ctrl, 0x90008010);
    WriteU32(op.cmd_wdg_cnt_clr, 0x000035CA);
    return op;
}

on_fw_operation InitOnFwOperation() {
    on_fw_operation op{};
    op.addr_smwp_enable[0] = 0xA2;
    WriteU32(op.addr_program_reload_from, 0x00000000);
    op.addr_raw_out_sel[0] = 0x98;
    WriteU32(op.addr_flash_checksum, 0x80000044);
    WriteU32(op.data_flash_checksum, 0x00000491);
    WriteU32(op.addr_crc_value, 0x80000050);
    WriteU32(op.addr_set_frame_addr, 0x10007294);
    op.addr_fw_mode_status[0] = 0x99;
    WriteU32(op.addr_icid_addr, 0x900000D0);
    op.addr_fw_ver_start[0] = 0x90;
    WriteU32(op.data_safe_mode_release_pw_active, 0x00000053);
    WriteU32(op.data_safe_mode_release_pw_reset, 0x00000000);
    WriteU32(op.data_clear, 0x00000000);
    op.addr_criteria_addr[0] = 0x9A;
    op.data_selftest_pass[0] = 0xAA;
    WriteU32(op.addr_reK_crtl, 0x8000000C);
    op.data_reK_en[0] = 0x02;
    op.data_reK_dis[0] = 0xFD;
    op.data_rst_init[0] = 0xF0;
    op.data_dc_set[0] = 0x02;
    op.data_bank_set[0] = 0x03;
    op.addr_selftest_addr_en[0] = 0x98;
    op.addr_selftest_result_addr[0] = 0x9B;
    op.data_selftest_request[0] = 0x06;
    op.data_thx_avg_mul_dc_lsb[0] = 0x22;
    op.data_thx_avg_mul_dc_msb[0] = 0x0B;
    op.data_thx_mul_dc_up_low_bud[0] = 0x64;
    op.data_thx_avg_slf_dc_lsb[0] = 0x14;
    op.data_thx_avg_slf_dc_msb[0] = 0x05;
    op.data_thx_slf_dc_up_low_bud[0] = 0x64;
    op.data_thx_slf_bank_up[0] = 0x40;
    op.data_thx_slf_bank_low[0] = 0x00;
    op.data_idle_dis_pwd[0] = 0x40;
    op.data_idle_en_pwd[0] = 0x00;
    op.data_rawdata_ready_hb[0] = 0xA3;
    op.data_rawdata_ready_lb[0] = 0x3A;
    op.addr_ahb_addr[0] = 0x11;
    op.data_ahb_dis[0] = 0x00;
    op.data_ahb_en[0] = 0x01;
    op.addr_event_addr[0] = 0x30;
    op.addr_usb_detect[0] = 0xA4;
    return op;
}

on_flash_operation InitOnFlashOperation() {
    on_flash_operation op{};
    WriteU32(op.addr_ctrl_base, 0x80000000);
    WriteU32(op.addr_ctrl_auto, 0x80000004);
    WriteU32(op.data_main_erase, 0x0000A50D);
    op.data_auto[0] = 0xA5;
    op.data_main_read[0] = 0x03;
    op.data_page_write[0] = 0x05;
    op.data_sfr_read[0] = 0x14;
    op.data_spp_read[0] = 0x10;
    WriteU32(op.addr_ahb_ctrl, 0x80000020);
    WriteU32(op.data_ahb_squit, 0x00000001);
    WriteU32(op.addr_unlock_0, 0x00000000);
    WriteU32(op.addr_unlock_4, 0x00000004);
    WriteU32(op.addr_unlock_8, 0x00000008);
    WriteU32(op.addr_unlock_c, 0x0000000C);
    WriteU32(op.data_cmd0, 0x28178EA0);
    WriteU32(op.data_cmd1, 0x0A0E03FF);
    WriteU32(op.data_cmd2, 0x8C203D0C);
    WriteU32(op.data_cmd3, 0x00300263);
    WriteU32(op.data_lock, 0x03400000);
    return op;
}

on_sram_operation InitOnSramOperation() {
    on_sram_operation op{};
    WriteU32(op.addr_rawdata_addr, 0x10000400);
    WriteU32(op.addr_rawdata_end, 0x00000000);
    WriteU32(op.data_conti, 0x44332211);
    WriteU32(op.data_fin, 0x00000000);
    WriteU32(op.passwrd, 0x00005AA5);
    return op;
}

on_driver_operation InitOnDriverOperation() {
    on_driver_operation op{};
    WriteU32(op.addr_fw_define_int_is_edge, 0x10007088);
#if defined(HX_NEW_EVENT_STACK_FORMAT)
    WriteU32(op.addr_fw_rx_tx_maxpt_num, 0x0800001C);
    WriteU32(op.addr_fw_xy_rev_int_edge, 0x0800000C);
    WriteU32(op.addr_fw_define_x_y_res, 0x08000030);
    WriteU32(op.addr_fw_maxpt_bt_num, 0x0800000C);
#else
    WriteU32(op.addr_fw_rx_tx_maxpt_num, 0x08000004);
    WriteU32(op.addr_fw_xy_rev_int_edge, 0x08000110);
    WriteU32(op.addr_fw_define_x_y_res, 0x08000010);
#endif
    WriteU32(op.data_fw_define_rxnum_txnum_maxpt_sorting, 0x00000008);
    WriteU32(op.data_fw_define_rxnum_txnum_maxpt_normal, 0x00000014);
    op.data_df_rx[0] = 0x1C;
    op.data_df_tx[0] = 0x0E;
    op.data_df_pt[0] = 0x0A;
    return op;
}

bool burst_enable(HalDevice* dev, uint8_t state) {
    if (!dev || !dev->IsValid()) return false;
    
    bool res = false;
    uint8_t tmp_data[4];

    tmp_data[0] = (0x12 | state);

    res = dev->WriteBus(0x0D, NULL, tmp_data, 1);
    if (!res) {
        dev->GetError();
        return false;
    }
    
    tmp_data[0] = 0x31;
    
    res = dev->WriteBus(0x13, NULL, tmp_data, 1);
    if (!res) {
        dev->GetError();
        return false;
    }
    
    return res;
    
}

bool register_read(HalDevice* dev, const uint8_t* addr, uint8_t* buffer, uint32_t len) {
    if (!dev || !dev->IsValid()) return false;
    bool res  = false;

    const uint8_t addr_ahb_addr_byte_0 = 0x00;
    const uint8_t addr_ahb_access_direction = 0x0C;
    const uint8_t data_ahb_access_direction_read[] = { 0x00 };
    const uint8_t addr_ahb_rdata_byte_0 = 0x08;

    res = dev->WriteBus(addr_ahb_addr_byte_0, addr, NULL, 0);
    if (!res) {
        dev->GetError();
        return false;
    }

    res = dev->WriteBus(addr_ahb_access_direction, NULL, 
        data_ahb_access_direction_read, 1);
    if (!res) {
        dev->GetError();
        return false;
    }

    res = dev->ReadBus(addr_ahb_rdata_byte_0, buffer, len);
    if (!res) {
        dev->GetError();
        return false;
    }
    return res;
}

void build_local_60_packet(uint8_t param_1, uint8_t param_3, uint8_t* output_buffer) {
    // 1. 初始化缓冲区为 0 (对应 local_5b = 0, local_50 = 0 等初始化)
    for (int i = 0; i < 16; i++) {
        output_buffer[i] = 0;
    }

    // 2. 设置包含“虚拟包头”的初始值，用于计算 CheckSum
    output_buffer[0] = 0xA8;     // local_60[0]
    output_buffer[1] = 0x8A;     // local_60[1]
    output_buffer[2] = param_1;  // local_60[2]
    output_buffer[3] = 0x00;     // local_60[3]
    output_buffer[4] = param_3;  // local_60[4]
    
    // 3. 计算 CheckSum (iVar13)
    // 逻辑：将 16 字节看作 8 个 uint16 (小端序) 进行累加
    uint32_t checksum_sum = 0;
    
    for (int i = 0; i < 16; i += 2) {
        // 组合低字节和高字节 (Little Endian)
        // bVar3 = buffer[i]; *pbVar1 = buffer[i+1];
        uint16_t word = output_buffer[i] | (output_buffer[i+1] << 8);
        checksum_sum += word;
    }

    // 4. 计算最终校验值 (对应: 0x10000 - iVar13)
    // 注意：这里只取低16位有效值
    uint16_t final_checksum = (uint16_t)(0x10000 - checksum_sum);

    // 5. 填入 CheckSum 到 buffer 末尾 (Index 14, 15)
    // 根据分析：是 Little Endian 写入
    output_buffer[14] = final_checksum & 0xFF;        // 低字节 (0x4A)
    output_buffer[15] = (final_checksum >> 8) & 0xFF; // 高字节 (0x75)

    // 6. 关键步骤：发送前清除包头
    output_buffer[0] = 0x00;
    output_buffer[1] = 0x00;
}

bool register_write(HalDevice* dev, const uint8_t* addr, const uint8_t* val, uint32_t len) {
    if (!dev || !dev->IsValid()) return false;

    if (len > 4) {
        burst_enable(dev, 1);
    } else {
        burst_enable(dev, 0);
    }

    return dev->WriteBus(0x00, addr, val, len);
}

bool write_and_verify(HalDevice* dev, const uint8_t* addr, const uint8_t* data, uint32_t len, uint32_t verify_len = 0) {
    if (!dev || !dev->IsValid()) return false;

    auto to_hex = [](const uint8_t* src, uint32_t size) {
        std::string out;
        out.reserve(size * 3);
        for (uint32_t i = 0; i < size; ++i) {
            out += std::format("{:02X}", src[i]);
            if (i + 1 < size) out += ' ';
        }
        return out;
    };

    std::vector<uint8_t> write_buf(data, data + len);
    std::vector<uint8_t> read_buf(len, 0);

    if (!register_write(dev, addr, data, len)) {
        HIMAX_LOG("bus access failed");
        return false;
    }

    if (!register_read(dev, addr, read_buf.data(), len)) {
        HIMAX_LOG("bus access failed");
        return false;
    }

    uint32_t cmp_len = verify_len;
    if (cmp_len == 0) {
        cmp_len = (len >= 2) ? 2u : len;
    }
    if (cmp_len > len) {
        cmp_len = len;
    }

    if (std::equal(write_buf.begin(), write_buf.begin() + cmp_len, read_buf.begin())) {
        return true;
    }

    const std::string expect = to_hex(write_buf.data(), cmp_len);
    const std::string actual = to_hex(read_buf.data(), cmp_len);
    std::string message = std::format("expect {} but read {}", expect, actual);
    HIMAX_LOG(message);
    return false;
}

bool safeModeSetRaw_intf(HalDevice* dev, const bool state) {
    if (!dev || !dev->IsValid()) return false;
    uint8_t tmp_data[2]{};
    const uint8_t adr_i2c_psw_lb = 0x31;

    if (state == 1) {
        tmp_data[0] = 0x27;
        tmp_data[1] = 0x95;
        if (!dev->WriteBus(adr_i2c_psw_lb, nullptr, tmp_data, 2)) return false;
        if (!dev->ReadBus(adr_i2c_psw_lb, tmp_data, 2)) return false;

        const bool ok = (tmp_data[0] == 0x27 && tmp_data[1] == 0x95);
        std::string message = ok
            ? std::format("Enter safe mode!")
            : std::format("Enter safe mode failed: 0x{:02X} 0x{:02X}", tmp_data[0], tmp_data[1]);
        HIMAX_LOG(message);
        return ok;
    } else {
        if (!dev->WriteBus(adr_i2c_psw_lb, nullptr, tmp_data, 2)) return false;
        if (!dev->ReadBus(adr_i2c_psw_lb, tmp_data, 2)) return false;

        const bool ok = (tmp_data[0] == 0x00 && tmp_data[1] == 0x00);
        std::string message = ok
            ? std::format("Leave safe mode!")
            : std::format("Leave safe mode failed: 0x{:02X} 0x{:02X}", tmp_data[0], tmp_data[1]);
        HIMAX_LOG(message);
        return ok;
    }
}
} // namespace

HalDevice* Chip::SelectDevice(DeviceType type) {
    switch (type) {
    case DeviceType::Master: return m_master.get();
    case DeviceType::Slave: return m_slave.get();
    case DeviceType::Interrupt: return m_interrupt.get();
    default: return nullptr;
    }
}

bool Chip::IsReady(DeviceType type) const {
    switch (type) {
    case DeviceType::Master: return m_master && m_master->IsValid();
    case DeviceType::Slave: return m_slave && m_slave->IsValid();
    case DeviceType::Interrupt: return m_interrupt && m_interrupt->IsValid();
    default: return false;
    }
}

bool Chip::check_bus(void) {
    bool slave_ok = false;
    bool master_ok = false;
    uint8_t tmp_data[4]{};
    uint8_t tmp_data1[4]{};

    // Check Slave
    if (m_slave->ReadBus(0x13, tmp_data, 1) && tmp_data[0] != 0) {
        slave_ok = true;
        message = std::format("Slave Bus Alive! (0x13: 0x{:02X})", tmp_data[0]);
    } else {
        message = std::format("Slave Bus Dead! (0x13: 0x{:02X})", tmp_data[0]);
    }
    HIMAX_LOG(message);

    // Check Master
    if (m_master->ReadBus(0x13, tmp_data1, 1) && tmp_data1[0] != 0) {
        master_ok = true;
        message = std::format("Master Bus Alive! (0x13: 0x{:02X})", tmp_data1[0]);
    } else {
        message = std::format("Master Bus Dead! (0x13: 0x{:02X})", tmp_data1[0]);
    }
    HIMAX_LOG(message);

    return slave_ok && master_ok;
}

bool Chip::init_buffers_and_register(void) {
    std::vector<uint8_t> tmp_data(0x50, 0);
    uint8_t tmp_register[4] = {0x50, 0x75, 0x00, 0x10};
    uint8_t tmp_register2[4] = {0x3c, 0x75, 0x00, 0x10};
    register_write(m_master.get(), tmp_register, tmp_data.data(), 0x50);
    register_write(m_master.get(), tmp_register2, m_on_fw_op.data_clear, 4);

    return true;
}

bool Chip::send_and_check_command(uint8_t param_1, uint8_t param_3) {
    std::vector<uint8_t> tmp_data(16, 0);
    
    uint8_t tmp_addr[] = {0x50, 0x75, 0x00, 0x10};
    
    for (int i = 0; i < 3; i++) {
        build_local_60_packet(param_1, param_3, tmp_data.data());
        register_write(m_master.get(), tmp_addr, tmp_data.data(), tmp_data.size());

        tmp_data[0] = 0xa8;
        tmp_data[1] = 0x8a;
        register_write(m_master.get(), tmp_addr, tmp_data.data(), 4);
        tmp_addr[0] = tmp_addr[0] + 0x10;
    }

    return true;
}

bool Chip::thp_afe_clear_status(uint8_t param_1) {
    return send_and_check_command(6, param_1);
}

bool Chip::hx_set_raw_data_type(DeviceType device, uint32_t type) {
    HalDevice* dev = SelectDevice(device);
    if (!dev || !dev->IsValid()) return false;

    std::vector<uint8_t> tmp_data(4, 0);
    
    switch (type) {
        case 0x100: tmp_data[0] = 0x0B; break;
        case 0x101:
        case 0x102: tmp_data[0] = 0x0A; break;
        case 0x103: tmp_data[0] = 0x0F; break;
        case 0x105: tmp_data[0] = 0xF6; break;
        default:    tmp_data[0] = type; break;
    }

    bool step_ok = false;
    for (int i = 0; i < 10; ++i) {
        if (write_and_verify(dev, m_fw_op.addr_raw_out_sel, tmp_data.data(), 4, 1)) {
            step_ok = true;
            break;
        }
    }

    if (!step_ok) {
        message = std::format("SetRawDataType: Failed to set mode 0x{:02X} to 0x100072EC", tmp_data[0]);
        HIMAX_LOG(message);
        return false;
    }

    step_ok = false;
    for (int i = 0; i < 10; ++i) {
        // 修正：passwrd 长度为 2 字节
        if (register_write(dev, m_sram_op.addr_rawdata_addr, m_on_sram_op.passwrd, 4)) {
            step_ok = true;
            break;
        }
        Sleep(1);
    }

    if (!step_ok) {
        HIMAX_LOG("Failed to write SRAM password (0x5AA5)");
    }

    message = std::format("set raw data out select: 0x{:02X}", type);
    HIMAX_LOG(message);
    return true;
}

bool Chip::hx_hw_reset_ahb_intf(DeviceType type) {
    HalDevice* dev = SelectDevice(type);
    if (!dev || !dev->IsValid()) return false;

    bool res = true;
    message = std::format("Enter!");
    HIMAX_LOG(message);

    bool step_ok = register_write(dev, m_driver_op.addr_fw_define_2nd_flash_reload, m_fw_op.data_clear, 4);
    message = std::format("clear reload done {}", step_ok ? "succed" : "failed");
    HIMAX_LOG(message);
    res = res && step_ok;

    step_ok = dev->SetReset(0);
    if (!step_ok) HIMAX_LOG("SetReset(0) failed");
    res = res && step_ok;

    step_ok = dev->SetReset(1);
    if (!step_ok) HIMAX_LOG("SetReset(1) failed");
    res = res && step_ok;

    step_ok = burst_enable(dev, 1);
    if (!step_ok) HIMAX_LOG("burst_enable set to 1 failed");
    res = res && step_ok;

    return res;
}

bool Chip::hx_sw_reset_ahb_intf(DeviceType type) {
    HalDevice* dev = SelectDevice(type);
    if (!dev || !dev->IsValid()) return false;

    // 尝试5次进入safe mode，每次尝试sleep(10)
    bool safe_mode_ok = false;
    for (int i = 0; i < 5; ++i) {
        if (safeModeSetRaw_intf(dev, true)) {
            safe_mode_ok = true;
            break;
        }
        Sleep(10);
    }

    // 若失败，打印日志，继续运行
    if (!safe_mode_ok) {
        HIMAX_LOG("Failed to enter Safe Mode before reset, proceeding anyway...");
    }
    
    Sleep(10);

    // 清空addr_fw_define_2nd_flash_reload，不尝试，失败输出clean reload done failed!，立即返回
    if (!register_write(dev, m_driver_op.addr_fw_define_2nd_flash_reload, m_driver_op.data_fw_define_flash_reload_en, 4)) {
        HIMAX_LOG("clean reload done failed!");
        return false;
    }

    Sleep(10);

    // addr_system_reset，不尝试，失败直接返回"Failed to write System Reset command"
    if (!register_write(dev, m_fw_op.addr_system_reset, m_fw_op.data_system_reset, 4)) {
        HIMAX_LOG("Failed to write System Reset command");
        return false;
    }

    Sleep(100);
    
    burst_enable(dev, 1);

    return true;
}

bool Chip::hx_reload_set(uint8_t state) {
    bool res = false;
    const bool enable = (state != 0);
    uint8_t tmp_data[4] = {0x5A, 0xA5, 0x00, 0x00};
    if (!enable) {
        tmp_data[0] = 0x00;
        tmp_data[1] = 0x00;
    }

    message = std::format("set reload {}", enable ? "enable" : "disable");
    HIMAX_LOG(message);

    for (int attempt = 0; attempt < 9; ++attempt) {
        res = write_and_verify(m_master.get(), m_driver_op.addr_fw_define_flash_reload, tmp_data, 4);
        if (res) break;
    }

    message = std::format("set reload {}", res ? "succeed" : "failed");
    HIMAX_LOG(message);
    return res;
}

bool Chip::hx_set_N_frame(uint8_t nFrame) {
    if (!m_master || !m_master->IsValid()) return false;

    std::array<uint8_t, 4> tmp_data{};

    const uint32_t target1 = static_cast<uint32_t>(nFrame);
    const uint32_t target2 = 0x7F0C0000u + static_cast<uint32_t>(nFrame);

    auto pack32 = [&](uint32_t v) {
        tmp_data[0] = static_cast<uint8_t>(v & 0xFFu);
        tmp_data[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
        tmp_data[2] = static_cast<uint8_t>((v >> 16) & 0xFFu);
        tmp_data[3] = static_cast<uint8_t>((v >> 24) & 0xFFu);
    };

    pack32(target1);
    if (!write_and_verify(m_master.get(), m_fw_op.addr_set_frame_addr, tmp_data.data(), 4)) return false;

    pack32(target2);
    if (!write_and_verify(m_master.get(), m_fw_op.addr_set_frame_addr, tmp_data.data(), 4)) return false;
    return true;
}

bool Chip::hx_switch_mode(uint32_t mode) {
    constexpr int kUnlockAttempts = 20;
    constexpr int kWriteAttempts = 20;

    bool clear = false;
    clear = write_and_verify(m_master.get(), m_sram_op.addr_rawdata_addr, m_sram_op.addr_rawdata_end, 4, 2);
    if (!clear) {
        message = std::format("switch mode unlock failed after {} attempts", kUnlockAttempts);
        HIMAX_LOG(message);
        return false;
    }

    uint8_t tmp_data[4]{};
    const uint8_t* src = nullptr;
    switch (mode) {
    case 0x100: // normal
        src = m_fw_op.data_normal_mode;
        break;
    case 0x101: // open
        src = m_fw_op.data_open_mode;
        break;
    case 0x102: // short
        src = m_fw_op.data_short_mode;
        break;
    case 0x103: // sorting
        src = m_fw_op.data_sorting_mode;
        break;
    default:
        message = std::format("undefined mode: 0x{:02X}, fallback to normal", mode);
        HIMAX_LOG(message);
        src = m_fw_op.data_normal_mode;
        break;
    }

    for (size_t i = 0; i < 4; ++i) {
        tmp_data[i] = src[i];
    }

    const uint32_t password =
        static_cast<uint32_t>(tmp_data[0]) |
        (static_cast<uint32_t>(tmp_data[1]) << 8) |
        (static_cast<uint32_t>(tmp_data[2]) << 16) |
        (static_cast<uint32_t>(tmp_data[3]) << 24);

    bool wrote = false;
    for (int attempt = 0; attempt < kWriteAttempts; ++attempt) {
        if (write_and_verify(m_master.get(), m_fw_op.addr_sorting_mode_en, tmp_data, 4)) {
            wrote = true;
            break;
        }
        Sleep(10);
    }

    if (wrote) {
        message = std::format("switch mode password(0x{:08X}) written!", password);
    } else {
        message = std::format("switch mode write failed after {} attempts, password(0x{:08X})", kWriteAttempts, password);
    }
    HIMAX_LOG(message);
    return wrote;
}

bool Chip::hx_is_reload_done_ahb(void) {
    std::vector<uint8_t> tmp_data(4, 0);
    bool step = register_read(m_master.get(), m_driver_op.addr_fw_define_2nd_flash_reload, tmp_data.data(), 4);
    if (step && tmp_data[0] == 0xC0 && tmp_data[1] == 0x72) {
        return true;
    } else {
        return false;
    }
}

bool Chip::hx_sense_on(bool isHwReset) {
    message = std::format("hx_sense_on: isHwReset = {}", isHwReset);
    HIMAX_LOG(message);

    // 1. 尝试进入 Safe Mode (Sense Off)
    if (!safeModeSetRaw_intf(m_master.get(), true)) {
        message = "safeModeSetRaw(true) failed";
        HIMAX_LOG(message);
        // 如果失败，尝试硬件复位救活
        if (isHwReset) {
            hx_hw_reset_ahb_intf(DeviceType::Master);
        }
        return false;
    }

    // 2. 配置参数
    std::vector<uint8_t> tmp_data(4, 0);
    bool step_ok = hx_set_N_frame(1);
    if (!step_ok) {
        message = std::format("hx_set_N_frame(1) failed!");
    }

    step_ok = hx_reload_set(0);
    if (!step_ok) {
        message = std::format("hx_reload_set (false) failed");
    }
    //hx_mode待找出,还要添加试错和sleep
    step_ok = hx_switch_mode(hx_mode);

    // 3. 执行复位，并读取addr_sts_chk，尝试5次
    uint8_t attempt = 0;
    do {
        if (isHwReset) {
            hx_hw_reset_ahb_intf(DeviceType::Master);
        } else {
            // 0x90000018 = 0x55 (Software Reset)
            hx_sw_reset_ahb_intf(DeviceType::Master);
        }
        //
        step_ok = register_read(m_master.get(), m_zf_op.addr_sts_chk, tmp_data.data(), 4);
        if (step_ok) {
            if (tmp_data[0] == 0x05) {
                message = std::format("sts_chk check success1");
                HIMAX_LOG(message);
                break;
            } else {
                message = std::format("sts_chk: 0x{:02X}", tmp_data[0]);
                HIMAX_LOG(message);
            }
        } else {
            step_ok = false;
            attempt++;
        }
    }while (attempt < 5);
    
    if (step_ok) {
        for (int retry = 0; retry < 50; retry++) {
            if (hx_is_reload_done_ahb()) {
                step_ok = true;
                break;
            }
            Sleep(1);
        }
    
        if (step_ok) {
            hx_set_raw_data_type(DeviceType::Master, hx_mode);
            hx_set_raw_data_type(DeviceType::Slave, hx_mode);
            Sleep(0x14);
            HIMAX_LOG("OUT!");
            return true;
        }
    //这里的else逻辑还有待确认
    } else {
        tmp_data.clear();
        tmp_data.resize(4, 0);
        step_ok = register_read(m_master.get(), m_fw_op.addr_reload_status, tmp_data.data(), 4);
        if (step_ok) {
            message = std::format("reload status 0x{:8x} = 0x{:X}", 0x80050000, tmp_data[0]);
            HIMAX_LOG(message);
        }
        HIMAX_LOG("OUT!");
        return false;
    }
    
    HIMAX_LOG("hx_sense_on timeout");
    return false; // 启动超时
}

void Chip::thp_afe_start(void) {
    hx_hw_reset_ahb_intf(DeviceType::Slave);
    hx_hw_reset_ahb_intf(DeviceType::Master);

    //不尝试，成功SpiSetReset set high,失败SpiSetReset set high failed!
    m_master->SetReset(1);

    Sleep(0x19);
    bool THP_AFE_STATE = check_bus();
    if (!THP_AFE_STATE) {
        throw ERROR;
    }
    burst_enable(m_master.get(), 1);
    
    //initbuffer&register

        //向10007550写入长为80的0数组用于清空
        
        //清空完后向3c750010写入4位0数组

        //设置寄存器与缓冲区，退出initbuffer&register
    
    init_buffers_and_register();
    
    //hx_sense_on
    hx_sense_on(true);
    m_master->IntOpen();
    m_slave->IntOpen();
    //打开两个设备的中断

    //清空buffer
    std::vector<uint8_t> tmp_buffer1(5063);
    std::vector<uint8_t> tmp_buffer2(339);

    for (int cnt = 0; cnt < 3; cnt++) {
        if (m_slave->GetFrame(tmp_buffer2.data(), 339, nullptr)) {
            m_slave->IntOpen();
        }
        if (m_master->GetFrame(tmp_buffer1.data(), 5063, nullptr)) {
            m_master->IntOpen();
        }
        
    }
    
    //更新状态
    thp_afe_clear_status(0x08);

}

bool Chip::WaitInterrupt() {
    if (m_interrupt && m_interrupt->IsValid()) {
        return m_interrupt->WaitInterrupt();
    }
    return false;
}

bool Chip::GetMasterData(std::vector<uint8_t>& buffer) {
    if (m_master && m_master->IsValid()) {
        uint32_t retLen = 0;
        return m_master->GetFrame(buffer.data(), static_cast<uint32_t>(buffer.size()), &retLen);
    }
    return false;
}

bool Chip::GetSlaveData(std::vector<uint8_t>& buffer) {
    if (m_slave && m_slave->IsValid()) {
        uint32_t retLen = 0;
        return m_slave->GetFrame(buffer.data(), static_cast<uint32_t>(buffer.size()), &retLen);
    }
    return false;
}
} // namespace Himax