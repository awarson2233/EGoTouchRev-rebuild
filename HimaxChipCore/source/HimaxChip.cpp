#include "HimaxChip.h"
#include <array>
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
    : m_ic_op(InitIcOperation()),
      m_fw_op(InitFwOperation()),
      m_flash_op(InitFlashOperation()),
      m_sram_op(InitSramOperation()),
      m_driver_op(InitDriverOperation()),
      m_zf_op(InitZfOperation())
{
    m_master = std::make_unique<HalDevice>(master_path.c_str(), DeviceType::Master);
    m_slave = std::make_unique<HalDevice>(slave_path.c_str(), DeviceType::Slave);
    m_interrupt = std::make_unique<HalDevice>(interrupt_path.c_str(), DeviceType::Interrupt);
    
    m_inspection_mode = THP_INSPECTION_ENUM::HX_RAWDATA;
    afe_mode = THP_AFE_MODE::Normal;

    std::string message;
    current_slot = 0;
}

/**
 * @brief 根据设备类型选择对应的 HalDevice 指针
 * @param type 设备类型 (Master/Slave/Interrupt)
 * @return HalDevice* 有效设备指针，若无效或类型错误则返回 nullptr
 */
HalDevice* Chip::SelectDevice(DeviceType type) {
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
        return nullptr;
    }
    return (dev && dev->IsValid()) ? dev : nullptr;
}

/**
 * @brief 检查指定类型的设备是否已就绪且有效
 * @param type 设备类型
 * @return bool 是否就绪
 */
bool Chip::IsReady(DeviceType type) const {
    return const_cast<Chip*>(this)->SelectDevice(type) != nullptr;
}

/**
 * @brief 检查主从设备的总线连接状态
 * @return bool 主从设备总线均正常返回 true
 */
bool Chip::check_bus(void) {
    bool slave_ok = false;
    bool master_ok = false;
    uint8_t tmp_data[4]{};
    uint8_t tmp_data1[4]{};

    // Check Slave
    if (m_slave->ReadBus(0x13, tmp_data, 1) && tmp_data[0] != 0) {
        slave_ok = true;
        HIMAX_LOG(std::format("Slave Bus Alive! (0x13: 0x{:02X})", tmp_data[0]));
    } else {
        HIMAX_LOG(std::format("Slave Bus Dead! (0x13: 0x{:02X})", tmp_data[0]));
    }

    // Check Master
    if (m_master->ReadBus(0x13, tmp_data1, 1) && tmp_data1[0] != 0) {
        master_ok = true;
        HIMAX_LOG(std::format("Master Bus Alive! (0x13: 0x{:02X})", tmp_data1[0]));
    } else {
        HIMAX_LOG(std::format("Master Bus Dead! (0x13: 0x{:02X})", tmp_data1[0]));
    }

    return slave_ok && master_ok;
}

/**
 * @brief 初始化缓冲区和寄存器设置
 * @return bool 是否成功
 */
bool Chip::init_buffers_and_register(void) {
    std::vector<uint8_t> tmp_data(0x50, 0);
    uint8_t tmp_data1[4]{0};

    uint32_t tmp_register = 0x10007550;
    uint32_t tmp_register2 = 0x1000753C;
    HimaxProtocol::register_write(m_master.get(), tmp_register, tmp_data.data(), 0x50);
    HimaxProtocol::register_write(m_master.get(), tmp_register2, tmp_data1, 4);
    current_slot = 0x00;

    return true;
}

bool Chip::himax_mcu_assign_sorting_mode(uint8_t* tmp_data) {
    bool step_ok = false;
    for (int i = 0; i < 10; ++i) {
        if (HimaxProtocol::write_and_verify(m_master.get(), m_fw_op.addr_sorting_mode_en, tmp_data, 4)) {
            step_ok = true;
            break;
        }
    }
    if (!step_ok) {
        HIMAX_LOG(std::format("Failed to assign sorting mode!"));
        return false;
    }
    return true;
}

bool Chip::hx_set_raw_data_type(DeviceType device, THP_INSPECTION_ENUM mode) {
    HalDevice* dev = SelectDevice(device);
    if (!dev) return false;
    std::string tmp_string;
    std::vector<uint8_t> tmp_data(4, 0);
    
    switch (mode) {
    case THP_INSPECTION_ENUM::HX_RAWDATA:
        tmp_data[0] = 0xF6;
        tmp_string = "HX_RAWDATA";
        break;
    case THP_INSPECTION_ENUM::HX_ACT_IDLE_RAWDATA:
        tmp_data[0] = 0x0A;
        tmp_string = "HX_ACT_IDLE_RAWDATA";
        break;
    case Himax::THP_INSPECTION_ENUM::HX_LP_IDLE_RAWDATA:
        tmp_data[0] = 0x0A;
        tmp_string = "HX_LP_IDLE_RAWDATA";
        break;

    case THP_INSPECTION_ENUM::HX_BACK_NORMAL:
        tmp_data[0] = 0x00;
        tmp_string = "HX_BACK_NORMAL";
        break;

    default:
        tmp_data[0] = 0xF6;
        tmp_string = "HX_RAWDATA";
        break;
    }

    bool step_ok = false;
    for (int i = 0; i < 10; ++i) {
        if (HimaxProtocol::write_and_verify(dev, m_fw_op.addr_raw_out_sel, tmp_data.data(), 4, 1)) {
            step_ok = true;
            break;
        }
    }

    if (!step_ok) {
        HIMAX_LOG(std::format("SetRawDataType: Failed to set mode 0x{:02X} to 0x100072EC", tmp_data[0]));
        return false;
    }

    step_ok = false;
    for (int i = 0; i < 10; ++i) {
        // 修正：passwrd 长度为 2 字节
        uint8_t tmp_data2[4];
        himax_parse_assign_cmd(m_sram_op.passwrd, tmp_data2, 4);
        if (HimaxProtocol::register_write(dev, m_sram_op.addr_rawdata_addr, tmp_data2, 4)) {
            step_ok = true;
            break;
        }
        Sleep(1);
    }

    if (!step_ok) {
        HIMAX_LOG("Failed to write SRAM password (0x5AA5)");
    }

    HIMAX_LOG(std::format("set raw data out select: {:s}", tmp_string));
    return true;
}

/**
 * @brief 执行硬件复位 AHB 接口
 * @param type 设备类型
 * @return bool 是否成功
 */
bool Chip::hx_hw_reset_ahb_intf(DeviceType type) {
    HalDevice* dev = SelectDevice(type);
    if (!dev) return false;
    uint8_t tmp_data[4];
    
    bool res = true;
    HIMAX_LOG("Enter!");

    himax_parse_assign_cmd(m_fw_op.data_clear, tmp_data, 4);
    bool step_ok = HimaxProtocol::register_write(dev, m_driver_op.addr_fw_define_2nd_flash_reload, tmp_data, 4);
    HIMAX_LOG(std::format("clear reload done {}", step_ok ? "succed" : "failed"));
    res = res && step_ok;

    step_ok = dev->SetReset(0);
    if (!step_ok) {
        HIMAX_LOG("SetReset(0) failed");
    }
    res = res && step_ok;

    step_ok = dev->SetReset(1);
    if (!step_ok) {
        HIMAX_LOG("SetReset(1) failed");
    }
    res = res && step_ok;

    step_ok = HimaxProtocol::burst_enable(dev, 1);
    if (!step_ok) {
        HIMAX_LOG("burst_enable set to 1 failed");
    }
    res = res && step_ok;

    return res;
}

/**
 * @brief 执行软件复位 AHB 接口
 * @param type 设备类型
 * @return bool 是否成功
 */
bool Chip::hx_sw_reset_ahb_intf(DeviceType type) {
    HalDevice* dev = SelectDevice(type);
    if (!dev) return false;
    uint8_t tmp_data[4];

    // 尝试5次进入safe mode，每次尝试sleep(10)
    bool safe_mode_ok = false;
    for (int i = 0; i < 5; ++i) {
        if (HimaxProtocol::safeModeSetRaw(dev, true)) {
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
    himax_parse_assign_cmd(m_driver_op.data_fw_define_flash_reload_en, tmp_data, 4);
    if (!HimaxProtocol::register_write(dev, m_driver_op.addr_fw_define_2nd_flash_reload, tmp_data, 4)) {
        HIMAX_LOG("clean reload done failed!");
        return false;
    }
    Sleep(10);
    
    // addr_system_reset，不尝试，失败直接返回"Failed to write System Reset command"
    himax_parse_assign_cmd(m_fw_op.data_system_reset, tmp_data, 4);
    if (!HimaxProtocol::register_write(dev, m_fw_op.addr_system_reset, tmp_data, 4)) {
        HIMAX_LOG("Failed to write System Reset command");
        return false;
    }

    Sleep(100);

    HimaxProtocol::burst_enable(dev, 1);

    return true;
}

/**
 * @brief 设置 Flash 重载状态
 * @param state 状态 (0 为禁用, 非 0 为启用)
 * @return bool 是否成功
 */
bool Chip::hx_reload_set(uint8_t state) {
    std::string message;
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
        res = HimaxProtocol::write_and_verify(m_master.get(), m_driver_op.addr_fw_define_flash_reload, tmp_data, 4);
        if (res) break;
    }

    message = std::format("set reload {}", res ? "succeed" : "failed");
    HIMAX_LOG(message);
    return res;
}

/**
 * @brief 设置 N 帧参数
 * @param nFrame 帧数
 * @return bool 是否成功
 */
bool Chip::hx_set_N_frame(uint8_t nFrame) {
    if (!m_master || !m_master->IsValid()) return false;
    HIMAX_LOG("Enter!");

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
    if (!HimaxProtocol::write_and_verify(m_master.get(), m_fw_op.addr_set_frame_addr, tmp_data.data(), 4)) return false;

    pack32(target2);
    if (!HimaxProtocol::write_and_verify(m_master.get(), m_fw_op.addr_set_frame_addr, tmp_data.data(), 4)) return false;
    HIMAX_LOG("Out!");
    return true;
}

bool Chip::himax_switch_mode_inspection(THP_INSPECTION_ENUM mode) {
    std::string message;
    constexpr int kUnlockAttempts = 20;
    constexpr int kWriteAttempts = 20;
    std::array<uint8_t, 4> tmp_data{};
    bool clear = false;

    himax_parse_assign_cmd(m_sram_op.addr_rawdata_end, tmp_data.data(), 4);
    for (size_t i = 0; i < 20; i++) {
        clear = HimaxProtocol::write_and_verify(m_master.get(), m_sram_op.addr_rawdata_addr, tmp_data.data(), 4, 2);
        if (clear) {
            break;
        }
    }
    if (!clear) {
        message = std::format("switch mode unlock failed after {} attempts", kUnlockAttempts);
        HIMAX_LOG(message);
        return false;
    }

    tmp_data.fill(0);
    switch (mode) {
    case THP_INSPECTION_ENUM::HX_RAWDATA: // normal
        tmp_data[0] = 0x00;
        tmp_data[1] = 0x00;
        break;
    case THP_INSPECTION_ENUM::HX_ACT_IDLE_RAWDATA: // open
        tmp_data[0] = 0x22;
        tmp_data[1] = 0x22;
        break;
    case THP_INSPECTION_ENUM::HX_LP_IDLE_RAWDATA: // short
        tmp_data[0] = 0x50;
        tmp_data[1] = 0x50;
        break;
    default:
        message = std::format("undefined mode, fallback to normal");
        HIMAX_LOG(message);
        tmp_data[0] = 0x00;
        tmp_data[0] = 0x00;
        break;
    }

    const uint32_t password =
        static_cast<uint32_t>(tmp_data[0]) |
        (static_cast<uint32_t>(tmp_data[1]) << 8) |
        (static_cast<uint32_t>(tmp_data[2]) << 16) |
        (static_cast<uint32_t>(tmp_data[3]) << 24);

    bool wrote = false;
    for (int attempt = 0; attempt < kWriteAttempts; ++attempt) {
        if (HimaxProtocol::write_and_verify(m_master.get(), m_fw_op.addr_sorting_mode_en, tmp_data.data(), 4)) {
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

/**
 * @brief 检查 Flash 重载是否完成
 * @return bool 完成返回 true
 */
bool Chip::hx_is_reload_done_ahb(void) {
    std::vector<uint8_t> tmp_data(4, 0);
    bool step = HimaxProtocol::register_read(m_master.get(), m_driver_op.addr_fw_define_2nd_flash_reload, tmp_data.data(), 4);
    if (step && tmp_data[0] == 0xC0 && tmp_data[1] == 0x72) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief 开启感应模式
 * @param isHwReset 是否执行硬件复位
 * @return bool 是否成功
 */
bool Chip::hx_sense_on(bool isHwReset) {
    std::string message;
    message = std::format("hx_sense_on: isHwReset = {}", isHwReset);
    HIMAX_LOG(message);

    // 1. 尝试进入 Safe Mode (Sense Off)
    if (!HimaxProtocol::safeModeSetRaw(m_master.get(), true)) {
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
    step_ok = himax_switch_mode_inspection(m_inspection_mode);

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
        step_ok = HimaxProtocol::register_read(m_master.get(), m_zf_op.addr_sts_chk, tmp_data.data(), 4);
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
    } while (attempt < 5);

    if (step_ok) {
        for (int retry = 0; retry < 50; retry++) {
            if (hx_is_reload_done_ahb()) {
                step_ok = true;
                break;
            }
            Sleep(1);
        }

        if (step_ok) {
            hx_set_raw_data_type(DeviceType::Master, m_inspection_mode);
            hx_set_raw_data_type(DeviceType::Slave, m_inspection_mode);
            Sleep(0x14);
            HIMAX_LOG("OUT!");
            return true;
        }
        //这里的else逻辑还有待确认
    } else {
        tmp_data.clear();
        tmp_data.resize(4, 0);
        step_ok = HimaxProtocol::register_read(m_master.get(), m_fw_op.addr_reload_status, tmp_data.data(), 4);
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

/**
 * @brief 关闭感应模式
 * @param check_en 是否检查固件状态
 * @return bool 是否成功
 */
bool Chip::hx_sense_off(bool check_en) {
    std::string message;
    bool step_ok = false;

    int cnt = 0;
    std::array<uint8_t, 4> send_data{};
    std::array<uint8_t, 4> back_data{};
    
    do {
        if (cnt == 0 || (back_data[0] != 0xA5 && back_data[0] != 0x00 && back_data[0] != 0x87)) {
            himax_parse_assign_cmd(m_fw_op.addr_ctrl_fw_isr, send_data.data(), 4);
            step_ok = HimaxProtocol::register_write(m_master.get(), m_fw_op.addr_ctrl_fw_isr, send_data.data(), 4);
        }
        Sleep(20);

        step_ok = HimaxProtocol::register_read(m_master.get(), m_fw_op.addr_chk_fw_status, back_data.data(), 4);
        if ((back_data[0] != 0x05) || (check_en == false)) { 
            message = std::format("Do not need wait FW, status = 0x{:X}", back_data[0]);
            HIMAX_LOG(message);
            break;
        }

        HimaxProtocol::register_read(m_master.get(), m_fw_op.addr_ctrl_fw_isr, send_data.data(), 4);

    }while (send_data[0] != 0x87 && (++cnt < 20) && check_en);


    cnt = 0;
    back_data.fill(0);
    do {
        for (int i = 0; i < 5; i++) {
            step_ok = HimaxProtocol::safeModeSetRaw(m_master.get(), false);
            if (step_ok) break;
        }

        step_ok = HimaxProtocol::register_read(m_master.get(), m_fw_op.addr_chk_fw_status, back_data.data(), 4);
        message = std::format("Check enter_safe_mode data[0]={:x}", back_data[0]);
        HIMAX_LOG(message);

        if (back_data[0] == 0x0C) {
            //reset TCON
            himax_parse_assign_cmd(m_ic_op.addr_tcon_on_rst, send_data.data(), 4);
            step_ok = HimaxProtocol::register_write(m_master.get(), m_ic_op.addr_tcon_on_rst, send_data.data(), 4);
            
            //原厂驱动没有 reset ADC，保留
            /* 
            step_ok = register_write(m_master.get(), m_ic_op.addr_adc_on_rst, m_ic_op.data_rst, 4);
            
            std::array<uint8_t, 4> tmp_data = {0x01, 0x00, 0x00, 0x00};
            step_ok = register_write(m_master.get(), m_ic_op.addr_adc_on_rst, tmp_data.data(), 4);
            */
            return true;
        }
        m_master->SetReset(0);
        Sleep(20);
        m_master->SetReset(50);
        Sleep(50);
    }while (cnt++ < 15);

    message = std::format("Out!");
    HIMAX_LOG(message);
    return step_ok;
}

/**
 * @brief 统一的 AFE 模式切换接口
 * @param cmd AFE 命令类型
 * @param param 可选参数 (用于 ClearStatus 等需要参数的命令)
 * @return bool 是否成功
 */
bool Chip::switch_afe_mode(AFE_Command cmd, uint8_t param) {
    switch (cmd) {
    case AFE_Command::ClearStatus:
        // TODO: 原 thp_afe_clear_status 逻辑
        // 使用 HimaxProtocol::send_command(m_master.get(), cmd_id, cmd_val, current_slot);
        break;
        
    case AFE_Command::EnableFreqShift:
        // TODO: 原 thp_afe_enable_freq_shift 逻辑
        break;
        
    case AFE_Command::DisableFreqShift:
        // TODO: 原 thp_afe_disable_freq_shift 逻辑
        break;
        
    case AFE_Command::StartCalibration:
        // TODO: 原 thp_afe_start_calibration 逻辑
        break;
        
    case AFE_Command::EnterIdle:
        // TODO: 原 thp_afe_enter_idle 逻辑
        break;
        
    default:
        return false;
    }
    return true;
}

bool Chip::thp_afe_start(void) {
    bool step_ok = false;
    uint8_t cnt = 0;

    hx_hw_reset_ahb_intf(DeviceType::Slave);
    hx_hw_reset_ahb_intf(DeviceType::Master);

    m_master->SetReset(1);
    Sleep(0x19);

    if (!check_bus()) {
        return false;
        //exit
    }
    HimaxProtocol::burst_enable(m_master.get(), 1);

    HIMAX_LOG("init");
    init_buffers_and_register();
    hx_sense_on(true);
    m_master->IntOpen();
    m_slave->IntOpen();

    return true;
}

void thp_afe_frame_solve(std::array<uint8_t, 8192>* frame_buffer) {
    uint32_t i = 5;
    for ( ; i < 4805; ) {
        uint8_t highBytes = frame_buffer->at(i);
        uint8_t lowBytes = frame_buffer->at(++i);

        uint16_t rawData = (highBytes << 8) | lowBytes;
        if (rawData < 33568 && rawData > 27768) {
            rawData = 0;
        }
    }
}

bool Chip::TouchEngine() {
    std::array<uint8_t, 8192> frame_buffer{};
    THP_AFE_MODE last_mode;

    uint32_t master_len = 0;
    uint32_t slave_len = 0;
    //开启
    thp_afe_start();
    
    //工作
    while (isRuning) {
        if (afe_mode != last_mode) {
            
        }
        m_interrupt->WaitInterrupt();
        m_slave->GetFrame(frame_buffer.data() + 5120, 339, &slave_len);
        m_master->GetFrame(frame_buffer.data(), 5063, &master_len);

        thp_afe_frame_solve(&frame_buffer);
    }

    //退出
    return hx_sense_off(false);
}
} // namespace Himax
