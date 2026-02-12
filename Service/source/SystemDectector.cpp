#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <windows.h>

class Listener {
public:
    // Bits set in *event_mask correspond to the index in event_list (0..7).
    // Example: bit0 => MonitorPowerOnEvent, bit1 => MonitorPowerOffEvent, etc.
    explicit Listener(std::atomic<std::uint32_t>* event_mask);
    ~Listener();

    // Waits until any monitored system event is signaled.
    // When an event is received, sets the corresponding bit in *event_mask.
    void worker();

private:
    static constexpr std::size_t kEventCount = 8;
    std::array<HANDLE, kEventCount> event_list{};
    std::atomic<std::uint32_t>* event_mask_ = nullptr;
};

namespace {
SECURITY_ATTRIBUTES BuildPermissiveGlobalSa(SECURITY_DESCRIPTOR& sd) {
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = nullptr;

    if (InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) != 0) {
        // Null DACL => full access for everyone. Useful for Global\\ named objects.
        if (SetSecurityDescriptorDacl(&sd, TRUE, nullptr, FALSE) != 0) {
            sa.lpSecurityDescriptor = &sd;
        }
    }

    return sa;
}

HANDLE CreateNamedManualResetEvent(LPSECURITY_ATTRIBUTES sa, const wchar_t* name) {
    return CreateEventW(sa, /*bManualReset*/ TRUE, /*bInitialState*/ FALSE, name);
}
} // namespace

Listener::Listener(std::atomic<std::uint32_t>* event_mask) : event_mask_(event_mask) {
    SECURITY_DESCRIPTOR sd{};
    SECURITY_ATTRIBUTES sa = BuildPermissiveGlobalSa(sd);
    LPSECURITY_ATTRIBUTES sa_ptr = sa.lpSecurityDescriptor ? &sa : nullptr;

    event_list = {
        CreateNamedManualResetEvent(sa_ptr, L"Global\\MonitorPowerOnEvent"),
        CreateNamedManualResetEvent(sa_ptr, L"Global\\MonitorPowerOffEvent"),
        CreateNamedManualResetEvent(sa_ptr, L"Global\\MonitorConsoleDisplayOnEvent"),
        CreateNamedManualResetEvent(sa_ptr, L"Global\\MonitorConsoleDisplayOffEvent"),
        CreateNamedManualResetEvent(sa_ptr, L"Global\\MonitorLidOnEvent"),
        CreateNamedManualResetEvent(sa_ptr, L"Global\\MonitorLidOffEvent"),
        CreateNamedManualResetEvent(sa_ptr, L"Global\\MonitorShutDownEvent"),
        CreateNamedManualResetEvent(sa_ptr, L"Global\\PBT_APMRESUMEAUTOMATIC"),
    };
}

Listener::~Listener() {
    for (HANDLE& h : event_list) {
        if (h != nullptr && h != INVALID_HANDLE_VALUE) {
            CloseHandle(h);
        }
        h = nullptr;
    }
}

void Listener::worker() {
    // Wait until any of the events is signaled.
    DWORD wait_result = WaitForMultipleObjects(
        static_cast<DWORD>(event_list.size()),
        event_list.data(),
        /*bWaitAll*/ FALSE,
        /*dwMilliseconds*/ INFINITE);

    if (wait_result >= WAIT_OBJECT_0 && wait_result < WAIT_OBJECT_0 + event_list.size()) {
        const DWORD index = wait_result - WAIT_OBJECT_0;

        if (event_mask_ != nullptr && index < event_list.size()) {
            const std::uint32_t mask = (index < 32) ? (1u << index) : 0u;
            if (mask != 0u) {
                event_mask_->fetch_or(mask, std::memory_order_release);
            }
        }

        // Events are manual-reset; reset so subsequent signals can be detected cleanly.
        if (index < event_list.size() && event_list[index] != nullptr && event_list[index] != INVALID_HANDLE_VALUE) {
            ResetEvent(event_list[index]);
        }
    }
}