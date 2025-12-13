#include <cstddef>
#include <cstdint>
#include <errhandlingapi.h>
#include <fileapi.h>
#include <handleapi.h>
#include <ioapiset.h>
#include <minwinbase.h>
#include <minwindef.h>
#include <mutex>
#include <synchapi.h>
#include <vector>
#include <windows.h>
#include <winerror.h>
#include <winnt.h>
#include "HimaxCommon.h"

constexpr uint8_t OP_WRITE_MASTER = 0xF2;
constexpr uint8_t OP_READ_MASTER  = 0xF3;
constexpr uint8_t OP_WRITE_SLAVE  = 0xF4;
constexpr uint8_t OP_READ_SLAVE   = 0xF5;

const DWORD SPI_IOCTL_INT_OPEN    = 0x4001c00; // 打开中断/初始化
const DWORD SPI_IOCTL_INT_CLOSE   = 0x4001c04; // 关闭中断
const DWORD SPI_IOCTL_WRITEREAD   = 0x4001c10; // [不常用] 偶见于特定初始化流
const DWORD SPI_IOCTL_WAIT_INT    = 0x4001c20; // 等待中断触发
const DWORD SPI_IOCTL_FULL_DUPLEX = 0x4001c24; // [核心] BusRead / 全双工读
const DWORD SPI_IOCTL_GET_FRAME   = 0x4001c28; // 获取帧数据
const DWORD SPI_IOCTL_SET_TIMEOUT = 0x4001c2c; // 设置超时
const DWORD SPI_IOCTL_SET_BLOCK   = 0x4001c30; // 设置阻塞模式
const DWORD SPI_IOCTL_SET_RESET   = 0x4001c34; // 复位设备
const DWORD SPI_IOCTL_READ_ACPI   = 0x4001c38; // 读取 ACPI 配置

namespace Himax {

    class HalDevice {
    private:
        HANDLE m_handle;
        DWORD m_lastError;
        DeviceType m_type;

        uint8_t m_readOp;
        uint8_t m_writeOp;

        const size_t dummy_size = 1;
        const size_t header_size = 2;
        const size_t data_offset = header_size + dummy_size;

        std::vector<uint8_t> m_xfer_buffer;
        std::recursive_mutex m_mutex;
        
    public:
        HalDevice(const wchar_t* path, DeviceType type) : m_handle(INVALID_HANDLE_VALUE), m_lastError(0), m_type(type) {
            m_xfer_buffer.reserve(0x4000 + 32);

            m_handle = CreateFileW(
                path, 
                GENERIC_ALL, 
                FILE_SHARE_READ | FILE_SHARE_WRITE, 
                NULL, 
                OPEN_EXISTING, 
                FILE_FLAG_OVERLAPPED, 
                NULL
            );
            if (m_handle == INVALID_HANDLE_VALUE) {
                m_lastError = GetLastError();
            }
            if (type == DeviceType::Slave) {
                m_readOp = OP_READ_SLAVE;
                m_writeOp = OP_WRITE_SLAVE;
            } else {
                m_readOp = OP_READ_MASTER;
                m_writeOp = OP_WRITE_MASTER;
            }
        }

        ~HalDevice() {
            if (IsValid()) CloseHandle(m_handle);
        }

        bool IsValid() const {return m_handle != INVALID_HANDLE_VALUE;}

        template<typename Func>
        bool SyncIoOperation(Func ioFunc, DWORD expectedLen = 0, bool checkLen = false, DWORD* outBytes = NULL) {
            if (IsValid()) {
                m_lastError = ERROR_INVALID_HANDLE;
                return false;
            }
            OVERLAPPED ov = {0};
            ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (!ov.hEvent) {
                m_lastError = GetLastError();
                return false;
            }

            std::lock_guard<std::recursive_mutex> lock(m_mutex);

            DWORD bytesTransferred = 0;
            BOOL res = ioFunc(&ov, &bytesTransferred);

            if (!res) {
                DWORD err = GetLastError();
                if (err == ERROR_IO_PENDING) {
                    if (WaitForSingleObject(ov.hEvent, INFINITE) == WAIT_OBJECT_0) {
                        res = GetOverlappedResult(m_handle, &ov, &bytesTransferred, TRUE);
                        if (!res) m_lastError = GetLastError();
                    } else {
                        m_lastError = GetLastError();
                    }
                } else {
                    m_lastError = err;
                }
            }
            CloseHandle(ov.hEvent);

            if (!res) return false;
            if (checkLen && bytesTransferred != expectedLen) {
                // 实际写入长度与预期不符
                m_lastError = ERROR_WRITE_FAULT;
                return false;
            }
            
            if (outBytes) *outBytes = bytesTransferred;
            m_lastError = 0;
            return true;
        }

        bool Ioctl(DWORD code, const void* in, uint32_t inLen, void* out, uint32_t outLen, uint32_t* retLen) {
            bool result = false;
            for (int cnt = 0; cnt < 10; cnt++) {
                result = SyncIoOperation([&](OVERLAPPED* pov, LPDWORD pBytes) {
                    return DeviceIoControl(m_handle, code, (LPVOID)in, inLen, out, outLen, pBytes, pov);
                }, 0, false, (DWORD*)retLen);
                if (result) break;
            }

            return result;
        }

        bool Read(void* buffer, uint32_t len) {
            return SyncIoOperation([&](OVERLAPPED* pov, LPDWORD pBytes){
                return ReadFile(m_handle, buffer, len, pBytes, pov);
            });
        }

        bool WaitInterrupt() {
            if (!IsValid()) { m_lastError = ERROR_INVALID_HANDLE; return false; }

            // 中断等待通常是阻塞的，但这里用异步方式去“挂起”一个请求
            OVERLAPPED ov = {0};
            ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (!ov.hEvent) return false;

            BOOL res = FALSE;
            {
                std::lock_guard<std::recursive_mutex> lock(m_mutex);
                res = DeviceIoControl(m_handle, SPI_IOCTL_WAIT_INT, NULL, 0, NULL, 0, NULL, &ov);
            }

            if (!res && GetLastError() != ERROR_IO_PENDING) {
                m_lastError = GetLastError();
                CloseHandle(ov.hEvent);
                return false;
            }

            // 等待中断触发 (INFINITE 可能导致卡死，实际应用可考虑加超时)
            if (WaitForSingleObject(ov.hEvent, INFINITE) == WAIT_OBJECT_0) {
                DWORD bytes = 0;
                GetOverlappedResult(m_handle, &ov, &bytes, TRUE);
                res = TRUE;
            }
            else {
                m_lastError = GetLastError();
                res = FALSE;
            }

            CloseHandle(ov.hEvent);
            return res;
        }


        bool ReadBus(uint8_t cmd, uint8_t* data, uint32_t len) {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            bool res = false;
            size_t total_size = data_offset + len;

            m_xfer_buffer.clear();

            m_xfer_buffer.push_back(m_readOp);
            m_xfer_buffer.push_back(cmd);
            m_xfer_buffer.push_back(0x00);      //Dummy Byte
            
            m_xfer_buffer.resize(total_size, 0);
            
            uint32_t retLen = 0;
            res = Ioctl(SPI_IOCTL_FULL_DUPLEX, 
                             m_xfer_buffer.data(), m_xfer_buffer.size(), 
                             m_xfer_buffer.data(), m_xfer_buffer.size(), 
                             &retLen);
            
            if (res) {
                if (retLen >= total_size) {
                    memcpy(data, m_xfer_buffer.data() + data_offset, len);
                }
            }
            
            return res;
        }

        bool WriteBus(const uint8_t cmd, const uint8_t* addr, const uint8_t* data, const uint32_t len) {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);

            m_xfer_buffer.clear();
            m_xfer_buffer.push_back(m_writeOp);
            m_xfer_buffer.push_back(cmd);

            if (addr != NULL) {
                m_xfer_buffer.insert(m_xfer_buffer.end(), addr, addr + 4);
            }
            if (data != NULL) {
                m_xfer_buffer.insert(m_xfer_buffer.end(), data, data + len);
            }

            return SyncIoOperation([&](OVERLAPPED* pov, LPDWORD pBytes){
                return WriteFile(m_handle, m_xfer_buffer.data(), (DWORD)m_xfer_buffer.size(), NULL, pov);
            });
        }

        bool ReadAcpi(uint8_t* data, uint32_t len) {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            bool res = false;
            m_xfer_buffer.clear();
            
            uint32_t retLen = 0;

            res =  Ioctl(SPI_IOCTL_READ_ACPI, nullptr, 0, m_xfer_buffer.data(), len, &retLen);
            if (res) {
                if (retLen >= len) {
                    memcpy(data, m_xfer_buffer.data(), len);
                }
            }
            return res;
        }

        bool GetFrame(void* buffer, uint32_t outLen, uint32_t *retLen){
            return Ioctl(SPI_IOCTL_GET_FRAME, NULL, 0, buffer, outLen, retLen);
        }

        bool SetTimeOut(uint8_t milisecond) {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            *reinterpret_cast<uint32_t*>(m_xfer_buffer.data()) = static_cast<uint32_t>(milisecond);
            return Ioctl(SPI_IOCTL_SET_TIMEOUT, m_xfer_buffer.data(), 4, NULL, 0, NULL);
        }

        bool SetBlock(bool state) {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);

            m_xfer_buffer.clear();
            m_xfer_buffer.push_back(uint8_t(state));
            m_xfer_buffer.resize(4, 0);

            return Ioctl(SPI_IOCTL_SET_BLOCK, m_xfer_buffer.data(), 4, NULL, 0, NULL);          
        }

        bool SetReset(bool state) {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            
            m_xfer_buffer.clear();
            m_xfer_buffer.push_back(uint8_t(state));
            m_xfer_buffer.resize(4, 0);

            return Ioctl(SPI_IOCTL_SET_RESET, m_xfer_buffer.data(), 4, NULL, 0, NULL);  
        }

        bool IntOpen(void) {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            return Ioctl(SPI_IOCTL_INT_OPEN, NULL, 0, NULL, 0, NULL);
        }

        bool IntClose(void) {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            return Ioctl(SPI_IOCTL_INT_CLOSE, NULL, 0, NULL, 0, NULL);
        }

        DWORD GetError() {
            return m_lastError;
        }

    };
}