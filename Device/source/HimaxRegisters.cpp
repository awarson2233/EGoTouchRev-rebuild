#include "HimaxRegisters.h"

namespace {
/**
 * @brief 将32位整数以小端序写入4字节数组
 * @param dest 目标数组
 * @param value 要写入的值
 */
inline void WriteU32(uint8_t (&dest)[4], uint32_t value) noexcept {
    dest[0] = static_cast<uint8_t>((value >> 0) & 0xFFu);
    dest[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
    dest[2] = static_cast<uint8_t>((value >> 16) & 0xFFu);
    dest[3] = static_cast<uint8_t>((value >> 24) & 0xFFu);
}
}

namespace Himax {

ic_operation InitIcOperation() {
    ic_operation op{};
    op.addr_ahb_addr_byte_0 = 0x00;
    op.addr_ahb_rdata_byte_0 = 0x08;
    op.addr_ahb_access_direction = 0x0C;
    op.addr_conti = 0x13;
    op.addr_incr4 = 0x0D;
    op.adr_i2c_psw_lb = 0x31;
    op.adr_i2c_psw_ub = 0x32;
    op.data_ahb_access_direction_read = 0x00;
    op.data_conti = 0x31;
    op.data_incr4 = 0x12;
    op.data_i2c_psw_lb = 0x27;
    op.data_i2c_psw_ub = 0x95;
    op.addr_tcon_on_rst = 0x80020020;
    op.addr_adc_on_rst = 0x80020094;
    op.addr_psl = 0x900000A0;
    op.addr_cs_central_state = 0x900000A8;
    op.data_rst = 0x00000000;
    op.adr_osc_en = 0x900880A8;
    op.adr_osc_pw = 0x900880E0;
    return op;
}

fw_operation InitFwOperation() {
    fw_operation op{};
    op.addr_system_reset = 0x90000018;
    op.addr_ctrl_fw_isr = 0x9000005C;
    op.addr_flag_reset_event = 0x900000E4;
    op.addr_hsen_enable = 0x10007F14;
    op.addr_smwp_enable = 0x10007F10;
    op.addr_program_reload_from = 0x00000000;
    op.addr_program_reload_to = 0x08000000;
    op.addr_program_reload_page_write = 0x0000FB00;
    op.addr_raw_out_sel = 0x100072EC;
    op.addr_reload_status = 0x80050000;
    op.addr_reload_crc32_result = 0x80050018;
    op.addr_reload_addr_from = 0x80050020;
    op.addr_reload_addr_cmd_beat = 0x80050028;
    op.addr_selftest_addr_en = 0x10007F18;
    op.addr_criteria_addr = 0x10007F1C;
    op.addr_set_frame_addr = 0x10007294;
    op.addr_selftest_result_addr = 0x10007F24;
    op.addr_sorting_mode_en = 0x10007F04;
    op.addr_fw_mode_status = 0x10007088;
    op.addr_icid_addr = 0x900000D0;
    op.addr_fw_ver_addr = 0x10007004;
    op.addr_fw_cfg_addr = 0x10007084;
    op.addr_fw_vendor_addr = 0x10007000;
    op.addr_cus_info = 0x10007008;
    op.addr_proj_info = 0x10007014;
    op.addr_fw_state_addr = 0x900000F8;
    op.addr_fw_dbg_msg_addr = 0x10007F40;
    op.addr_chk_fw_status = 0x900000A8;
    op.addr_chk_dd_status = 0x900000E8;
    op.addr_dd_handshak_addr = 0x900000FC;
    op.addr_dd_data_addr = 0x10007F80;
    op.addr_clr_fw_record_dd_sts = 0x10007FCC;
    op.addr_ap_notify_fw_sus = 0x10007FD0;
    op.data_ap_notify_fw_sus_en = 0xA55AA55A;
    op.data_ap_notify_fw_sus_dis = 0x00000000;
    op.data_system_reset = 0x00000055;
    op.data_safe_mode_release_pw_active = 0x00000053;
    op.data_safe_mode_release_pw_reset = 0x00000000;
    op.data_clear = 0x00000000;
    op.data_fw_stop = 0x000000A5;
    op.data_program_reload_start = 0x0A3C3000;
    op.data_program_reload_compare = 0x04663000;
    op.data_program_reload_break = 0x15E75678;
    op.data_selftest_request = 0x00006AA6;
    op.data_criteria_aa_top = 0x64;
    op.data_criteria_aa_bot = 0x00;
    op.data_criteria_key_top = 0x64;
    op.data_criteria_key_bot = 0x00;
    op.data_criteria_avg_top = 0x64;
    op.data_criteria_avg_bot = 0x00;
    op.data_set_frame = 0x0000000A;
    op.data_selftest_ack_hb = 0xA6;
    op.data_selftest_ack_lb = 0x6A;
    op.data_selftest_pass = 0xAA;
    op.data_normal_cmd = 0x00;
    op.data_normal_status = 0x99;
    op.data_sorting_cmd = 0xAA;
    op.data_sorting_status = 0xCC;
    op.data_dd_request = 0xAA;
    op.data_dd_ack = 0xBB;
    op.data_idle_dis_pwd = 0x17;
    op.data_idle_en_pwd = 0x1F;
    op.data_rawdata_ready_hb = 0xA3;
    op.data_rawdata_ready_lb = 0x3A;
    op.addr_ahb_addr = 0x11;
    op.data_ahb_dis = 0x00;
    op.data_ahb_en = 0x01;
    op.addr_event_addr = 0x30;
    op.addr_usb_detect = 0x10007F38;
    op.addr_ulpm_33 = 0x33;
    op.addr_ulpm_34 = 0x34;
    op.data_ulpm_11 = 0x11;
    op.data_ulpm_22 = 0x22;
    op.data_ulpm_33 = 0x33;
    op.data_ulpm_aa = 0xAA;
    op.data_normal_mode = 0x00000000;
    op.data_open_mode = 0x00007777;
    op.data_short_mode = 0x00001111;
    op.data_sorting_mode = 0x0000AAAA;
    return op;
}

flash_operation InitFlashOperation() {
    flash_operation op{};
    op.addr_spi200_trans_fmt = 0x80000010;
    op.addr_spi200_trans_ctrl = 0x80000020;
    op.addr_spi200_fifo_rst = 0x80000030;
    op.addr_spi200_rst_status = 0x80000034;
    op.addr_spi200_flash_speed = 0x80000040;
    op.addr_spi200_cmd = 0x80000024;
    op.addr_spi200_addr = 0x80000028;
    op.addr_spi200_data = 0x8000002C;
    op.addr_spi200_bt_num = 0x800000E8;
    op.data_spi200_txfifo_rst = 0x00000004;
    op.data_spi200_rxfifo_rst = 0x00000002;
    op.data_spi200_trans_fmt = 0x00020780;
    op.data_spi200_trans_ctrl_1 = 0x42000003;
    op.data_spi200_trans_ctrl_2 = 0x47000000;
    op.data_spi200_trans_ctrl_3 = 0x67000000;
    op.data_spi200_trans_ctrl_4 = 0x610FF000;
    op.data_spi200_trans_ctrl_5 = 0x694002FF;
    op.data_spi200_trans_ctrl_6 = 0x42000000;
    op.data_spi200_trans_ctrl_7 = 0x6940020F;
    op.data_spi200_cmd_1 = 0x00000005;
    op.data_spi200_cmd_2 = 0x00000006;
    op.data_spi200_cmd_3 = 0x000000C7;
    op.data_spi200_cmd_4 = 0x000000D8;
    op.data_spi200_cmd_5 = 0x00000020;
    op.data_spi200_cmd_6 = 0x00000002;
    op.data_spi200_cmd_7 = 0x0000003B;
    op.data_spi200_cmd_8 = 0x00000003;
    op.data_spi200_addr = 0x00000000;
    return op;
}

sram_operation InitSramOperation() {
    sram_operation op{};
    op.addr_mkey = 0x100070E8;
    op.addr_rawdata_addr = 0x10000000;
    op.addr_rawdata_end = 0x00000000;
    op.passwrd = 0x0000A55A;
    return op;
}

driver_operation InitDriverOperation() {
    driver_operation op{};
    op.addr_fw_define_flash_reload = 0x10007F00;
    op.addr_fw_define_2nd_flash_reload = 0x100072C0;
    op.addr_fw_define_int_is_edge = 0x10007088;
    op.addr_fw_define_rxnum_txnum = 0x100070F4;
    op.addr_fw_define_maxpt_xyrvs = 0x100070F8;
    op.addr_fw_define_x_y_res = 0x100070FC;
    op.data_df_rx = 60;
    op.data_df_tx = 40;
    op.data_df_pt = 0x0A;
    op.data_fw_define_flash_reload_dis = 0x0000A55A;
    op.data_fw_define_flash_reload_en = 0x00000000;
    op.data_fw_define_rxnum_txnum_maxpt_sorting = 0x00000008;
    op.data_fw_define_rxnum_txnum_maxpt_normal = 0x00000014;
    return op;
}

zf_operation InitZfOperation() {
    zf_operation op{};
    op.data_dis_flash_reload = 0x00009AA9;
    op.addr_system_reset = 0x90000018;
    op.data_system_reset = 0x00000055;
    op.data_sram_start_addr = 0x08000000;
    op.data_cfg_info = 0x10007000;
    op.data_fw_cfg_1 = 0x10007084;
    op.data_fw_cfg_2 = 0x10007264;
    op.data_fw_cfg_3 = 0x10007300;
    op.data_adc_cfg_1 = 0x10006A00;
    op.data_adc_cfg_2 = 0x10007B28;
    op.data_adc_cfg_3 = 0x10007AF0;
    op.data_map_table = 0x10007500;
    op.addr_sts_chk = 0x900000A8;
    op.data_activ_sts = 0x05;
    op.addr_activ_relod = 0x90000048;
    op.data_activ_in = 0xEC;
    return op;
}

} // namespace Himax
