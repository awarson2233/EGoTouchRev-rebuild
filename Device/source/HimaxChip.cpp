/*
 * @Author: Detach0-0 detach0-0@outlook.com
 * @Date: 2025-12-05 11:45:27
 * @LastEditors: Detach0-0 detach0-0@outlook.com
 * @LastEditTime: 2026-01-09 16:37:22
 * @FilePath: \EGoTouchRev-vsc\HimaxChipCore\source\HimaxChip.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "HimaxChip.h"
#include "HimaxProtocol.h"
#include "Logger.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <string>
#include <synchapi.h>
#include <vector>
#include <windows.h>

namespace {

/**
 * @brief 将地址值按指定长度解析为小端序字节数组
 * @param addr 地址值
 * @param cmd 目标缓冲区
 * @param len 目标长度 (1, 2, 或 4)
 */
void himax_parse_assign_cmd(uint32_t addr, uint8_t *cmd, int len) {
    switch (len) {
    case 1:
        cmd[0] = static_cast<uint8_t>(addr & 0xFF);
        break;
    case 2:
        cmd[0] = static_cast<uint8_t>(addr & 0xFF);
        cmd[1] = static_cast<uint8_t>((addr >> 8) & 0xFF);
        break;
    case 4:
        cmd[0] = static_cast<uint8_t>(addr & 0xFF);
        cmd[1] = static_cast<uint8_t>((addr >> 8) & 0xFF);
        cmd[2] = static_cast<uint8_t>((addr >> 16) & 0xFF);
        cmd[3] = static_cast<uint8_t>((addr >> 24) & 0xFF);
        break;
    default:
        break;
    }
}

} // end anonymous namespace

namespace Himax {

Chip::Chip(const std::wstring& master_path, const std::wstring& slave_path, const std::wstring& interrupt_path)
    : pic_op(InitIcOperation()),
      pfw_op(InitFwOperation()),
      pflash_op(InitFlashOperation()),
      psram_op(InitSramOperation()),
      pdriver_op(InitDriverOperation()),
      pzf_op(InitZfOperation())
{
    m_master = std::make_unique<HalDevice>(master_path.c_str(), DeviceType::Master);
    m_slave = std::make_unique<HalDevice>(slave_path.c_str(), DeviceType::Slave);
    m_interrupt = std::make_unique<HalDevice>(interrupt_path.c_str(), DeviceType::Interrupt);
    
    m_inspection_mode = THP_INSPECTION_ENUM::HX_RAWDATA;
    afe_mode = THP_AFE_MODE::Normal;

    current_slot = 0;
}

/**
 * @brief 根据设备类型选择对应的 HalDevice 指针
 * @param type 设备类型 (Master/Slave/Interrupt)
 * @return HalDevice* 有效设备指针，若无效或类型错误则返回 nullptr
 */
ChipResult<HalDevice*> Chip::SelectDevice(DeviceType type) {
    HalDevice* dev = nullptr;
    switch (type) {
    case DeviceType::Master:
        dev = m_master.get();
        break;
    case DeviceType::Slave:
        dev = m_slave.get();
        break;
    case DeviceType::Interrupt:
        dev = m_interrupt.get();
        break;
    default:
        return std::unexpected(ChipError::InvalidOperation);
    }
    if (dev && dev->IsValid()) {
        return dev;
    }
    return std::unexpected(ChipError::CommunicationError);
}

/**
 * @brief 检查指定类型的设备是否已就绪且有效
 * @param type 设备类型
 * @return bool 是否就绪
 */
bool Chip::IsReady(DeviceType type) const {
    return const_cast<Chip*>(this)->SelectDevice(type).has_value();
}

/**
 * @brief 检查主从设备的总线连接状态
 * @return bool 主从设备总线均正常返回 true
 */
ChipResult<> Chip::check_bus(void) {
    uint8_t tmp_data[4]{};
    uint8_t tmp_data1[4]{};

    // Check Slave
    if (auto res = m_slave->ReadBus(0x13, tmp_data, 1); !res) {
        LOG_ERROR("Device", "Chip::check_bus", GetStateStr(), "Slave Bus Dead! Error: {:d}", (int)res.error());
        return std::unexpected(ChipError::CommunicationError);
    }
    LOG_INFO("Device", "Chip::check_bus", GetStateStr(), "Slave Bus Alive! (0x13: 0x{:02X})", tmp_data[0]);

    // Check Master
    if (auto res = m_master->ReadBus(0x13, tmp_data1, 1); !res) {
        LOG_ERROR("Device", "Chip::check_bus", GetStateStr(), "Master Bus Dead! Error: {:d}", (int)res.error());
        return std::unexpected(ChipError::CommunicationError);
    }
    LOG_INFO("Device", "Chip::check_bus", GetStateStr(), "Master Bus Alive! (0x13: 0x{:02X})", tmp_data1[0]);

    return {};
}

/**
 * @brief 初始化缓冲区和寄存器设置
 * @return bool 是否成功
 */
ChipResult<> Chip::init_buffers_and_register(void) {
    std::vector<uint8_t> tmp_data(0x50, 0);
    uint8_t tmp_data1[4]{0};

    uint32_t tmp_register = 0x10007550;
    uint32_t tmp_register2 = 0x1000753C;
    
    if (auto res = HimaxProtocol::register_write(m_master.get(), tmp_register, tmp_data.data(), 0x50); !res) return res;
    if (auto res = HimaxProtocol::register_write(m_master.get(), tmp_register2, tmp_data1, 4); !res) return res;
    
    current_slot = 0x00;
    return {};
}

ChipResult<> Chip::himax_mcu_assign_sorting_mode(uint8_t* tmp_data) {
    bool step_ok = false;
    for (int i = 0; i < 10; ++i) {
        if (HimaxProtocol::write_and_verify(m_master.get(), pfw_op.addr_sorting_mode_en, tmp_data, 4)) {
            step_ok = true;
            break;
        }
    }
    if (!step_ok) {
        LOG_ERROR("Device", "Chip::himax_mcu_assign_sorting_mode", GetStateStr(), "Failed to assign sorting mode!");
        return std::unexpected(ChipError::VerificationFailed);
    }
    return {};
}

ChipResult<> Chip::himax_switch_data_type(DeviceType device, THP_INSPECTION_ENUM mode) {
    std::array<uint8_t, 4> tmp_data{};
    std::string message;
    auto dev_res = SelectDevice(device);
    uint8_t cnt = 50;

    if (!dev_res) {
        LOG_ERROR("Device", "Chip::himax_switch_data_type", GetStateStr(), "get device failed");
        return std::unexpected(dev_res.error());
    }
    HalDevice* dev = *dev_res;

    switch (mode) {
    case THP_INSPECTION_ENUM::EGO_RAWDATA:
        tmp_data[0] = 0xF6;
        message = "EGO_RAWDATA";    
        break;
    
    case THP_INSPECTION_ENUM::HX_RAWDATA:
        tmp_data[0] = 0x0A;
        message = "HX_RAWDATA";
        break;

    case THP_INSPECTION_ENUM::HX_ACT_IDLE_RAWDATA:
        tmp_data[0] = 0x0A;
        message = "HX_ACT_IDLE_RAWDATA";
        break;
    
    case THP_INSPECTION_ENUM::HX_LP_IDLE_RAWDATA:
        tmp_data[0] = 0x0A;
        message = "HX_LP_IDLE_RAWDATA";
        break;
    
    case THP_INSPECTION_ENUM::HX_BACK_NORMAL:
        tmp_data[0] = 0x00;
        message = "HX_BACK_NORMAL";
        break;

    default:
        tmp_data[0] = 0x00;
        message = "HX_BACK_NORMAL_UNKNOW";
        break;
    }
    LOG_INFO("Device", "Chip::himax_switch_data_type", GetStateStr(), "switch to {}!", message);

    ChipResult<> step_ok = std::unexpected(ChipError::InternalError);
    do {
        step_ok = HimaxProtocol::write_and_verify(dev, pfw_op.addr_raw_out_sel, tmp_data.data(), 4);
    }while (!step_ok && --cnt > 0);

    if (step_ok) {
        LOG_INFO("Device", "Chip::himax_switch_data_type", GetStateStr(), "switch to {} success!", message);
        return {};
    } else {
        LOG_ERROR("Device", "Chip::himax_switch_data_type", GetStateStr(), "switch failed!");
        return step_ok;
    }
}

/**
 * @brief 执行硬件复位 AHB 接口
 * @param type 设备类型
 * @return bool 是否成功
 */
ChipResult<> Chip::hx_hw_reset_ahb_intf(DeviceType type) {
    auto dev_res = SelectDevice(type);
    if (!dev_res) return std::unexpected(dev_res.error());
    HalDevice* dev = *dev_res;

    uint8_t tmp_data[4];
    
    LOG_INFO("Device", "Chip::hx_hw_reset_ahb_intf", GetStateStr(), "Enter!");

    himax_parse_assign_cmd(pfw_op.data_clear, tmp_data, 4);
    if (auto res = HimaxProtocol::register_write(dev, pdriver_op.addr_fw_define_2nd_flash_reload, tmp_data, 4); !res) {
        LOG_ERROR("Device", "Chip::hx_hw_reset_ahb_intf", GetStateStr(), "clear reload failed");
        return res;
    }

    // 物理复位操作：经测试，Slave 句柄 (WinError 1168) 不支持复位控制，
    // 物理复位引脚仅绑定在 Master 句柄上，故统一由 m_master 执行。
    if (auto res = m_master->SetReset(0); !res) {
        LOG_ERROR("Device", "Chip::hx_hw_reset_ahb_intf", GetStateStr(), "Physical SetReset(0) via Master failed, WinError: {}", (int)m_master->GetError());
        return res;
    }

    if (auto res = m_master->SetReset(1); !res) {
        LOG_ERROR("Device", "Chip::hx_hw_reset_ahb_intf", GetStateStr(), "Physical SetReset(1) via Master failed, WinError: {}", (int)m_master->GetError());
        return res;
    }

    if (auto res = HimaxProtocol::burst_enable(dev, 1); !res) {
        LOG_ERROR("Device", "Chip::hx_hw_reset_ahb_intf", GetStateStr(), "burst_enable set to 1 failed");
        return res;
    }

    return {};
}

/**
 * @brief 执行软件复位 AHB 接口
 * @param type 设备类型
 * @return bool 是否成功
 */
ChipResult<> Chip::hx_sw_reset_ahb_intf(DeviceType type) {
    auto dev_res = SelectDevice(type);
    if (!dev_res) return std::unexpected(dev_res.error());
    HalDevice* dev = *dev_res;

    uint8_t tmp_data[4];

    // 尝试5次进入safe mode
    bool safe_mode_ok = false;
    for (int i = 0; i < 5; ++i) {
        if (HimaxProtocol::safeModeSetRaw(dev, true)) {
            safe_mode_ok = true;
            break;
        }
        Sleep(10);
    }

    if (!safe_mode_ok) {
        LOG_WARN("Device", "Chip::hx_sw_reset_ahb_intf", GetStateStr(), "Failed to enter Safe Mode before reset, proceeding anyway...");
    }

    Sleep(10);
    himax_parse_assign_cmd(pdriver_op.data_fw_define_flash_reload_en, tmp_data, 4);
    if (auto res = HimaxProtocol::register_write(dev, pdriver_op.addr_fw_define_2nd_flash_reload, tmp_data, 4); !res) {
        LOG_ERROR("Device", "Chip::hx_sw_reset_ahb_intf", GetStateStr(), "clean reload done failed!");
        return res;
    }
    Sleep(10);
    
    himax_parse_assign_cmd(pfw_op.data_system_reset, tmp_data, 4);
    if (auto res = HimaxProtocol::register_write(dev, pfw_op.addr_system_reset, tmp_data, 4); !res) {
        LOG_ERROR("Device", "Chip::hx_sw_reset_ahb_intf", GetStateStr(), "Failed to write System Reset command");
        return res;
    }

    Sleep(100);
    if (auto res = HimaxProtocol::burst_enable(dev, 1); !res) return res;

    return {};
}

/**
 * @brief 设置 Flash 重载状态
 * @param disable 状态 (0 为禁用, 非 0 为启用)
 * @return bool 是否成功
 */
ChipResult<> Chip::himax_mcu_reload_disable(uint8_t disable) {
    LOG_INFO("Device", "Chip::himax_mcu_reload_disable", GetStateStr(), "entering!");
    std::array<uint8_t, 4> tmp_data{};

    if (disable) {
        himax_parse_assign_cmd(pdriver_op.data_fw_define_flash_reload_dis, tmp_data.data(), 4);
    } else {
        himax_parse_assign_cmd(pdriver_op.data_fw_define_flash_reload_en, tmp_data.data(), 4);
    }
    
    if (auto res = HimaxProtocol::register_write(m_master.get(), pdriver_op.addr_fw_define_flash_reload, tmp_data.data(), 4); !res) {
        return res;
    }

    LOG_INFO("Device", "Chip::himax_mcu_reload_disable", GetStateStr(), "setting OK!");
    return {};
}

/**
 * @brief 设置 N 帧参数
 * @param nFrame 帧数
 * @return bool 是否成功
 */
ChipResult<> Chip::hx_set_N_frame(uint8_t nFrame) {
    if (!m_master || !m_master->IsValid()) return std::unexpected(ChipError::CommunicationError);
    LOG_INFO("Device", "Chip::hx_set_N_frame", GetStateStr(), "Enter!");

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
    if (auto res = HimaxProtocol::write_and_verify(m_master.get(), pfw_op.addr_set_frame_addr, tmp_data.data(), 4); !res) return res;

    pack32(target2);
    if (auto res = HimaxProtocol::write_and_verify(m_master.get(), pfw_op.addr_set_frame_addr, tmp_data.data(), 4); !res) return res;
    
    LOG_INFO("Device", "Chip::hx_set_N_frame", GetStateStr(), "Out!");
    return {};
}

ChipResult<> Chip::himax_switch_mode_inspection(THP_INSPECTION_ENUM mode) {
    std::string message;
    constexpr int kUnlockAttempts = 20;
    std::array<uint8_t, 4> tmp_data{};
    bool clear = false;
    LOG_INFO("Device", "Chip::himax_switch_mode_inspection", GetStateStr(), "Entering!");

    /*Stop Handshakng*/
    himax_parse_assign_cmd(psram_op.addr_rawdata_end, tmp_data.data(), 4);
    for (size_t i = 0; i < kUnlockAttempts; i++) {
        if (HimaxProtocol::write_and_verify(m_master.get(), psram_op.addr_rawdata_addr, tmp_data.data(), 4, 2)) {
            clear = true;
            break;
        }
    }
    if (!clear) {
        LOG_ERROR("Device", "Chip::himax_switch_mode_inspection", GetStateStr(), "switch mode unlock failed after {} attempts", kUnlockAttempts);
        return std::unexpected(ChipError::VerificationFailed);
    }

    /*Switch Mode*/
    tmp_data.fill(0);
    switch (mode) {
    case THP_INSPECTION_ENUM::HX_RAWDATA: // normal
        tmp_data[0] = 0x00;
        tmp_data[1] = 0x00;
        message = "HX_RAWDATA";
        break;
    case THP_INSPECTION_ENUM::HX_ACT_IDLE_RAWDATA: // open
        tmp_data[0] = 0x22;
        tmp_data[1] = 0x22;
        message = "HX_ACT_IDLE_RAWDATA";
        break;
    case THP_INSPECTION_ENUM::HX_LP_IDLE_RAWDATA: // short
        tmp_data[0] = 0x50;
        tmp_data[1] = 0x50;
        message = "HX_LP_IDLE_RAWDATA";
        break;
    case THP_INSPECTION_ENUM::HX_BACK_NORMAL:
        tmp_data[0] = 0x00;
        tmp_data[1] = 0x00;
        message = "HX_BACK_NORMAL";
        break;
    case THP_INSPECTION_ENUM::EGO_RAWDATA:
        tmp_data[0] = 0x00;
        tmp_data[1] = 0x00;
        message = "EGO_RAWDATA";
        break;
    }

    if (auto res = himax_mcu_assign_sorting_mode(tmp_data.data()); !res) {
        LOG_ERROR("Device", "Chip::himax_switch_mode_inspection", GetStateStr(), "Failed to switch to {}", message);
        return res;
    }
    
    LOG_INFO("Device", "Chip::himax_switch_mode_inspection", GetStateStr(), "Switching to {} Success", message);
    return {};
}

/**
 * @brief 检查 Flash 重载是否完成
 * @return bool 完成返回 true
 */
ChipResult<> Chip::hx_is_reload_done_ahb(void) {
    std::array<uint8_t, 4> tmp_data{};
    if (auto res = HimaxProtocol::register_read(m_master.get(), pdriver_op.addr_fw_define_2nd_flash_reload, tmp_data.data(), 4); !res) return res;
    
    if (tmp_data[0] == 0xC0 && tmp_data[1] == 0x72) {
        return {};
    }
    return std::unexpected(ChipError::InvalidOperation);
}

ChipResult<> Chip::himax_mcu_read_FW_status(void) {
    std::array<uint8_t, 4> tmp_data{};
    uint32_t dbg_reg_ary[4] = {pfw_op.addr_fw_dbg_msg_addr, pfw_op.addr_chk_fw_status,
	pfw_op.addr_chk_dd_status, pfw_op.addr_flag_reset_event};

    for (uint32_t addr : dbg_reg_ary) {
        if (auto res = HimaxProtocol::register_read(m_master.get(), addr, tmp_data.data(), 4); !res) return res;
        LOG_INFO("Device", "Chip::himax_mcu_read_FW_status", GetStateStr(), "{:x} = {::#x}",addr, tmp_data);
    }
    return {};
}

ChipResult<> Chip::himax_mcu_interface_on(void) {
    LOG_INFO("Device", "Chip::himax_mcu_interface_on", GetStateStr(), "Enter!");

    std::array<uint8_t, 4> tmp_data{};
    std::array<uint8_t, 4> tmp_data2{};
    int cnt = 0;
    
    if (auto res = m_master->ReadBus(pic_op.addr_ahb_rdata_byte_0, tmp_data.data(), 4); !res) {
        LOG_ERROR("Device", "Chip::himax_mcu_interface_on", GetStateStr(), "ReadBus failed");
        return res;
    }        
    
    do {
        tmp_data[0] = pic_op.data_conti;
        
        if (auto res = m_master->WriteBus(pic_op.addr_conti, NULL, tmp_data.data(), 1); !res) {
            LOG_ERROR("Device", "Chip::himax_mcu_interface_on", GetStateStr(), "bus access fail!");
            return res;
        }
        
        tmp_data[0] = pic_op.data_incr4;
        if (auto res = m_master->WriteBus(pic_op.addr_incr4, NULL, tmp_data.data(), 1); !res) {
            LOG_ERROR("Device", "Chip::himax_mcu_interface_on", GetStateStr(), "bus access fail!");
            return res;
        }

        if (auto res = m_master->ReadBus(pic_op.addr_conti, tmp_data.data(), 1); !res) return res;
        if (auto res = m_master->ReadBus(pic_op.addr_incr4, tmp_data2.data(), 1); !res) return res;

        if (tmp_data[0] == pic_op.data_conti && tmp_data2[0] == pic_op.data_incr4) {
            break;
        }

        Sleep(1);
    } while (++cnt < 10);

    if (cnt > 0) {
        LOG_INFO("Device", "Chip::himax_mcu_interface_on", GetStateStr(), "Polling burst mode: {:d} times", cnt);
    }
    return {};
}

/**
 * @brief 开启感应模式
 * @param FlashMode 是否执行硬件复位
 * @return bool 是否成功
 */
ChipResult<> Chip::hx_sense_on(bool FlashMode) {
    LOG_INFO("Device", "Chip::hx_sense_on", GetStateStr(), "Enter, isHwReset = {}", FlashMode);
    std::array<uint8_t, 4> tmp_data{};

    if (auto res = himax_mcu_interface_on(); !res) return res;
    himax_parse_assign_cmd(pfw_op.data_clear, tmp_data.data(), 4);
    if (auto res = HimaxProtocol::register_write(m_master.get(), pfw_op.addr_ctrl_fw_isr, tmp_data.data(), 4); !res) return res;
    
    Sleep(11);

    if (!FlashMode) {
        if (auto res = m_master->SetReset(false); !res) return res;
        if (auto res = m_master->SetReset(true); !res) return res;
    } else {
        tmp_data.fill(0);
        if (auto res = m_master->WriteBus(pic_op.adr_i2c_psw_lb, NULL, tmp_data.data(), 2); !res) return res;
    }
    return {};
}

/**
 * @brief 关闭感应模式
 * @param check_en 是否检查固件状态
 * @return bool 是否成功
 */
ChipResult<> Chip::hx_sense_off(bool check_en) {
    ChipResult<> step_ok = {};
    int cnt = 0;
    std::array<uint8_t, 4> send_data{};
    std::array<uint8_t, 4> back_data{};
    
    do {
        if (cnt == 0 || (back_data[0] != 0xA5 && back_data[0] != 0x00 && back_data[0] != 0x87)) {
            himax_parse_assign_cmd(pfw_op.data_fw_stop, send_data.data(), 4);
            step_ok = HimaxProtocol::register_write(m_master.get(), pfw_op.addr_ctrl_fw_isr, send_data.data(), 4);
            if (!step_ok) return step_ok;
        }
        Sleep(20);

        step_ok = HimaxProtocol::register_read(m_master.get(), pfw_op.addr_chk_fw_status, back_data.data(), 4);
        if (!step_ok) return step_ok;

        if ((back_data[0] != 0x05) || (check_en == false)) { 
            LOG_INFO("Device", "Chip::hx_sense_off", GetStateStr(), "Do not need wait FW, status = 0x{:X}", back_data[0]);
            break;
        }

        if (auto res = HimaxProtocol::register_read(m_master.get(), pfw_op.addr_ctrl_fw_isr, back_data.data(), 4); !res) return res;

    }while (send_data[0] != 0x87 && (++cnt < 20) && check_en);


    cnt = 0;
    back_data.fill(0);
    do {
        ChipResult<> safe_res = std::unexpected(ChipError::InternalError);
        for (int i = 0; i < 5; i++) {
            safe_res = HimaxProtocol::safeModeSetRaw(m_master.get(), true);
            if (safe_res) break;
        }

        if (auto res = HimaxProtocol::register_read(m_master.get(), pfw_op.addr_chk_fw_status, back_data.data(), 4); !res) return res;
        LOG_INFO("Device", "Chip::hx_sense_off", GetStateStr(), "Check enter_safe_mode data[0]={:x}", back_data[0]);

        if (back_data[0] == 0x0C) {
            //reset TCON
            himax_parse_assign_cmd(pic_op.addr_tcon_on_rst, send_data.data(), 4);
            if (auto res = HimaxProtocol::register_write(m_master.get(), pic_op.addr_tcon_on_rst, send_data.data(), 4); !res) return res;
            
            return {};
        }
        if (auto res = m_master->SetReset(0); !res) return res;
        Sleep(20);
        if (auto res = m_master->SetReset(1); !res) return res; // Fix: SetReset should be 1, original had 50? SetReset(bool)
        Sleep(50);
    }while (cnt++ < 15);

    LOG_INFO("Device", "Chip::hx_sense_off", GetStateStr(), "Out!");
    return std::unexpected(ChipError::VerificationFailed);
}

ChipResult<> Chip::himax_mcu_power_on_init(void) {
    LOG_INFO("Device", "Chip::himax_mcu_power_on_init", GetStateStr(), "entering!");
    std::array<uint8_t, 4> tmp_data{0x01, 0x00, 0x00, 0x00};
    std::array<uint8_t, 4> tmp_data2{};
    uint8_t retry = 0;

    if (auto res = HimaxProtocol::register_write(m_master.get(), pfw_op.addr_raw_out_sel, tmp_data2.data(), 4); !res) return res;
    if (auto res = HimaxProtocol::register_write(m_master.get(), pfw_op.addr_sorting_mode_en, tmp_data2.data(), 4); !res) return res;
    if (auto res = HimaxProtocol::register_write(m_master.get(), pfw_op.addr_set_frame_addr, tmp_data.data(), 4); !res) return res;
    if (auto res = HimaxProtocol::register_write(m_master.get(), pdriver_op.addr_fw_define_2nd_flash_reload, tmp_data2.data(), 4); !res) return res;

    if (auto res = hx_sense_on(false); !res) return res;

    LOG_INFO("Device", "Chip::himax_mcu_power_on_init", GetStateStr(), "waiting for FW reload data");

    while (retry++ < 30) {
        if (auto res = HimaxProtocol::register_read(m_master.get(), pdriver_op.addr_fw_define_2nd_flash_reload, tmp_data.data(), 4); !res) return res;
        if ((tmp_data[3] == 0x00 && tmp_data[2] == 0x00 &&
			tmp_data[1] == 0x72 && tmp_data[0] == 0xC0)) {
            LOG_INFO("Device", "Chip::himax_mcu_power_on_init", GetStateStr(), "FW reload done!");
            return {};
        }
        LOG_INFO("Device", "Chip::himax_mcu_power_on_init", GetStateStr(), "waiting for FW reload data %d", retry); 
        auto res = himax_mcu_read_FW_status(); // 打印 log
        Sleep(11);
    }
    LOG_ERROR("Device", "Chip::himax_mcu_power_on_init", GetStateStr(), "FW reload timeout!");
    return std::unexpected(ChipError::Timeout);
}

/**
 * @brief 进入低功耗模式
 * @param param 参数
 * @return bool 是否成功
 */
ChipResult<> Chip::thp_afe_enter_idle(uint8_t param) {
    if (m_connState.load() != ConnectionState::Connected) return std::unexpected(ChipError::InvalidOperation);
    LOG_INFO("Device", "Chip::thp_afe_enter_idle", GetStateStr(), "Entering!");

    // 2. 发送 EnterIdle 命令 (ID=0x0A, 使用传入的 param)
    if (auto res = HimaxProtocol::send_command(m_master.get(), 0x0a, 0x00, current_slot); !res) {
        LOG_ERROR("Device", "Chip::thp_afe_enter_idle", GetStateStr(), "Send ENTER_IDLE command failed!");
        return res;
    }

    // 3. 设置 SPI 超时 (对应 DLL 中的 SpiSetTimeout(200))
    auto res = m_master->SetTimeOut(200);
    if (m_slave) res = m_slave->SetTimeOut(200);

    // 4. 更新状态标志位
    afe_mode = THP_AFE_MODE::Idle;
    
    LOG_INFO("Device", "Chip::thp_afe_enter_idle", GetStateStr(), "Out!");
    return {};
}

/**
 * @brief 开始硬件校准
 * @param param 参数
 * @return bool 是否成功
 */
ChipResult<> Chip::thp_afe_start_calibration(uint8_t param) {
    if (m_connState.load() != ConnectionState::Connected) return std::unexpected(ChipError::InvalidOperation);
    LOG_INFO("Device", "Chip::thp_afe_start_calibration", GetStateStr(), "Entering!");

    // 发送 StartCalibration 命令 (ID=0x01, 使用传入的 param)
    if (auto res = HimaxProtocol::send_command(m_master.get(), 0x01, 0x00, current_slot); !res) {
        LOG_ERROR("Device", "Chip::thp_afe_start_calibration", GetStateStr(), "Send AFE_START_CALBRATION command failed!");
        return res;
    }

    LOG_INFO("Device", "Chip::thp_afe_start_calibration", GetStateStr(), "Out!");
    return {};
}

/**
 * @brief 强制退出低功耗模式
 */
ChipResult<> Chip::thp_afe_force_exit_idle(void) {
    if (m_connState.load() != ConnectionState::Connected) return std::unexpected(ChipError::InvalidOperation);
    LOG_INFO("Device", "Chip::thp_afe_force_exit_idle", GetStateStr(), "Entering!");
    
    // 1. 发送 ClearStatus (ID=0x06, Val=0x08)
    if (auto res = HimaxProtocol::send_command(m_master.get(), 0x0b, 0x00, current_slot); !res) return res;

    auto res = m_master->SetTimeOut(100); // 恢复默认超时
    afe_mode = THP_AFE_MODE::Normal;
    
    LOG_INFO("Device", "Chip::thp_afe_force_exit_idle", GetStateStr(), "Out!");
    return {};
}

ChipResult<> Chip::thp_afe_enable_freq_shift(void) {
    return HimaxProtocol::send_command(m_master.get(), 0x0d, 0x00, current_slot);
}

ChipResult<> Chip::thp_afe_disable_freq_shift(void) {
    return HimaxProtocol::send_command(m_master.get(), 0x02, 0x00, current_slot);
}

ChipResult<> Chip::thp_afe_clear_status(uint8_t cmd_val) {
    return HimaxProtocol::send_command(m_master.get(), 0x06, cmd_val, current_slot);
}

ChipResult<> Chip::thp_afe_force_to_freq_point(uint8_t freq_idx) {
    return HimaxProtocol::send_command(m_master.get(), 0x0c, freq_idx, current_slot);
}

ChipResult<> Chip::thp_afe_force_to_scan_rate(uint8_t rate_idx) {
    return HimaxProtocol::send_command(m_master.get(), 0x0e, 0, current_slot);
}

/**
 * @brief 统一的 AFE 模式切换接口
 * @param cmd AFE 命令类型
 * @param param 可选参数 (用于 ClearStatus 等需要参数的命令)
 * @return bool 是否成功
 */
ChipResult<> Chip::switch_afe_mode(AFE_Command cmd, uint8_t param) {
    switch (cmd) {
    case AFE_Command::ClearStatus:
        return thp_afe_clear_status(param);
    case AFE_Command::EnableFreqShift:
        return thp_afe_enable_freq_shift();
    case AFE_Command::DisableFreqShift:
        return thp_afe_disable_freq_shift();
    case AFE_Command::StartCalibration:
        return thp_afe_start_calibration(param);
    case AFE_Command::EnterIdle:
        return thp_afe_enter_idle(param);
    default:
        return std::unexpected(ChipError::InvalidOperation);
    }
}

ChipResult<> Chip::Init(void) {
    LOG_INFO("Device", "Chip::Init", GetStateStr(), "Starting initialization sequence...");

    std::array<uint8_t, 4> tmp_data = {0xA5, 0x5A, 0x00, 0x00};

    if (auto res = himax_mcu_reload_disable(false); !res) return res;
    if (auto res = himax_switch_mode_inspection(THP_INSPECTION_ENUM::EGO_RAWDATA); !res) return res;
    if (auto res = hx_set_N_frame(1); !res) return res;

    if (auto res = himax_mcu_power_on_init(); !res) return res;
    
    if (auto res = himax_switch_data_type(DeviceType::Master, THP_INSPECTION_ENUM::EGO_RAWDATA); !res) return res;
    if (auto res = HimaxProtocol::register_write(m_master.get(), psram_op.addr_rawdata_addr, tmp_data.data(), 4); !res) return res;
    if (auto res = himax_switch_data_type(DeviceType::Slave, THP_INSPECTION_ENUM::EGO_RAWDATA); !res) return res;
    if (auto res = HimaxProtocol::register_write(m_slave.get(), psram_op.addr_rawdata_addr, tmp_data.data(), 4); !res) return res;
    
    // 打开中断/数据采集通道
    if (auto res = m_master->IntOpen(); !res) {
        LOG_ERROR("Device", "Chip::Init", GetStateStr(), "Master IntOpen failed!");
        return res;
    }
    if (auto res = m_slave->IntOpen(); !res) {
        LOG_ERROR("Device", "Chip::Init", GetStateStr(), "Slave IntOpen failed!");
        return res;
    }

    m_connState.store(ConnectionState::Connected);
    LOG_INFO("Device", "Chip::Init", GetStateStr(), "Initialization and Sense ON successful.");
    return {};
}

ChipResult<> Chip::Deinit(void) {
    LOG_INFO("Device", "Chip::Deinit", GetStateStr(), "Starting Deinit sequence...");

    // Important: send sense off if bus is accessible
    auto resOff = hx_sense_off(false);
    if (!resOff) {
        LOG_WARN("Device", "Chip::Deinit", GetStateStr(), "hx_sense_off had issues during Deinit.");
    }
    
    auto m_res = m_master->IntClose();
    auto s_res = m_slave->IntClose();
    
    if (!m_res || !s_res) {
         LOG_WARN("Device", "Chip::Deinit", GetStateStr(), "IntClose had issues during Deinit.");
    }
    
    m_connState.store(ConnectionState::Unconnected);
    LOG_INFO("Device", "Chip::Deinit", GetStateStr(), "Deinit successful.");
    return {};
}

Chip::~Chip() {
    if (m_connState.load() != ConnectionState::Unconnected) {
        LOG_INFO("Device", "Chip::~Chip", GetStateStr(), "Implicitly calling Deinit().");
        (void)Deinit();
    }
}

ChipResult<> Chip::GetFrame(void) {
    if (m_connState.load() != ConnectionState::Connected) {
        return std::unexpected(ChipError::InvalidOperation);
    }

    // 从 Master 读取主帧数据 (5063 bytes)
    if (auto res = m_master->GetFrame(back_data.data(), 5063, nullptr); !res) {
        LOG_ERROR("Device", "Chip::GetFrame", GetStateStr(), "Master GetFrame failed!");
        return res;
    }

    // 从 Slave 读取副帧数据 (339 bytes)，拼接到 Master 之后
    if (auto res = m_slave->GetFrame(back_data.data() + 5063, 339, nullptr); !res) {
        LOG_ERROR("Device", "Chip::GetFrame", GetStateStr(), "Slave GetFrame failed!");
        return res;
    }

    return {};
}
} // namespace Himax