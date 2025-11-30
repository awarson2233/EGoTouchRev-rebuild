#include <cstdint>
#include <iostream>
#include <memory>
#include <windows.h>
#include <winnt.h>
#include <vector>
#include <condition_variable>
#include "HimaxChipCore.h"
#include "HimaxPlatform.h"

const WCHAR* DEVICE_PATH_INTERRUPT = L"\\\\.\\Global\\SPBTESTTOOL_MASTER";
const WCHAR* DEVICE_PATH_MASTER = L"\\\\.\\Global\\SPBTESTTOOL_MASTER";
const WCHAR* DEVICE_PATH_SLAVE = L"\\\\.\\Global\\SPBTESTTOOL_SLAVE";

const uint32_t HEADER_SIZE = 339;  // Slave 数据
const uint32_t BODY_SIZE = 5063; // Master 数据 (热力图)

std::unique_ptr<Himax::HalDevice> g_pMasterDev;	// DevID 0
std::unique_ptr<Himax::HalDevice> g_pIntDev;	// DevID 0
std::unique_ptr<Himax::HalDevice> g_pSlaveDev;		// DevID 1
std::unique_ptr<Himax::Control> g_pController;

std::vector<uint8_t> headerBuffer(HEADER_SIZE);
std::vector<uint8_t> bodyBuffer(BODY_SIZE);

std::condition_variable cv;		
std::mutex mtx;
bool g_DataReady = false;
bool g_Shutdown = false;

void SpiInterruptWaitThread(void) {
	while (!g_Shutdown) {
		DWORD byteReturned = 0;
		
		BOOL result = g_pIntDev->WaitInterrupt();

		if (!result && GetLastError() != ERROR_IO_PENDING) {
			std::cerr << "Error: Wait Interrupt Failed" << GetLastError() << std::endl;
			Sleep(1000);
			continue;
		}

		if (g_Shutdown) break;

		if (result)
		{
			std::lock_guard<std::mutex > lock(mtx);
			g_DataReady = true;
			cv.notify_one();
		}
		else
		{
			std::cerr << "[Thread A] Error: WaitForSingleObject FAILED. GetLastError: " << GetLastError() << std::endl;
		}
	}

}

void GetFrameThread(void) {
	uint32_t byteRead = 0;
	std::cout << "[Thread B] Data reader started. Waiting for signal..." << std::endl;

	while (!g_Shutdown) {
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [] {return g_DataReady || g_Shutdown; });

		if (g_Shutdown) break;

		if (g_pSlaveDev->GetFrame(headerBuffer.data(), HEADER_SIZE, &byteRead))
		{
			std::cout << "[Thread B] Read Header (" << byteRead << " bytes) OK." << std::endl;
		}
		else
		{
			std::cerr << "[Thread B] Read Header FAILED." << std::endl;
		}
		if (g_pMasterDev->GetFrame(bodyBuffer.data(), BODY_SIZE, &byteRead)) 
		{	
			std::cout << "[Thread B] Read body (" << byteRead << " bytes) OK." << std::endl;
			printf("  > Body[0-3]: %02X %02X %02X %02X\n",
				bodyBuffer[0], bodyBuffer[1], bodyBuffer[2], bodyBuffer[3]);
		}
		else
		{
			std::cerr << "[Thread B] Read Body FAILED." << std::endl;
		}

		g_DataReady = false;
		lock.unlock();
	}

	std::cout << "[Thread B] Data reader shutting down." << std::endl;
}

BOOL WINAPI ConsoleHandler(DWORD CEvent) {
	if (CEvent == CTRL_C_EVENT) {
		g_Shutdown = true;
		cv.notify_all();
		// 注意：Interrupt 线程可能还卡在 DeviceIoControl 里
		// 实际项目中可能需要 CancelIoEx，但这里简单处理
		return TRUE;
	}
	return FALSE;
}

int main(void) {
	g_pMasterDev = std::make_unique<Himax::HalDevice>(DEVICE_PATH_MASTER, Himax::DeviceType::Master);
	g_pSlaveDev = std::make_unique<Himax::HalDevice>(DEVICE_PATH_SLAVE, Himax::DeviceType::Slave);
	g_pIntDev = std::make_unique<Himax::HalDevice>(DEVICE_PATH_INTERRUPT, Himax::DeviceType::Interrupt);
	g_pController = std::make_unique<Himax::Control>(g_pMasterDev.get(), g_pSlaveDev.get(), g_pIntDev.get()); 
	
	if (!g_pMasterDev || !g_pSlaveDev || !g_pIntDev)
	{
		std::cout << "Device Open Error!" << std::endl;
		return 1;
	}

	std::cout << "All device handles opened successfully." << std::endl;

	std::cout << "Initialization skipped (assuming service is running). Starting threads." << std::endl << std::endl;
	/*
	// --- 3. 模拟阶段二：启动线程 ---
	std::thread threadA(SpiInterruptWaitThread);
	std::thread threadB(GetFrameThread);

	// 等待线程结束
	threadA.join();
	threadB.join();
	*/

/* 	g_pSlaveDev->SetBlock(true);
	g_pMasterDev->SetTimeOut(200);
	g_pSlaveDev->SetTimeOut(200);
	g_pMasterDev->SetReset(); */

	g_pController->CheckBusReady();

	g_pController->safeModeSetRaw_intf(1, 0);
	// --- 4. 清理 ---
	std::cout << "Closing handles..." << std::endl;
	system("pause");

}
