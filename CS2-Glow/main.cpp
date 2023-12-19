#include<Windows.h>
#include <Tlhelp32.h>
#include <thread>
#include <iostream>
#include<string>
using namespace std;

namespace  offsets
{
	DWORD64 dwLocalPlayer = 0x1810F58;
	DWORD64 dwEntityList = 0x17C1960;

	DWORD64 m_flDetectedByEnemySensorTime = 0x13E4; 
	DWORD64 m_hPlayerPawn = 0x7EC;
	DWORD64 m_iTeamNum = 0x3BF;
}

class Memory
{
public:
	const wchar_t* windowName = L"Counter-Strike 2";
	HWND hWnd; 
	HANDLE processHandle; 
	HANDLE snapShot; 
	DWORD processID;
	const wchar_t* moduleName = L"client.dll";
	MODULEENTRY32 entry; 
	DWORD64 client;

	Memory()
	{
		GetProcessID();
		client = (DWORD64)GetModuleAddress().modBaseAddr;
	}
	void GetProcessID()
	{
		do {
			hWnd = FindWindow(NULL, windowName);
			Sleep(50);
		} while (!hWnd);
		GetWindowThreadProcessId(hWnd, &processID);

		cout << "CS2 ProcessID: " << processID << endl;
		processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
		cout << "CS2 processHandle: " << processHandle << endl;
	};

	MODULEENTRY32 GetModuleAddress()
	{
		snapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processID);
		entry.dwSize = sizeof(MODULEENTRY32);

		if (snapShot != INVALID_HANDLE_VALUE) {
			if (Module32First(snapShot, &entry)) {
				do {
					if (!wcscmp((const wchar_t*)entry.szModule, moduleName)) {
						break;
					}
				} while (Module32Next(snapShot, &entry));
			}
			if (snapShot)
				CloseHandle(snapShot);
		}
		cout << "CS2 SnapShot: " << snapShot << endl;
		return entry;
	}

	template <typename T>
	T ReadMemory(DWORD64 address) {
		T buffer;

		ReadProcessMemory(processHandle, (LPCVOID)address, &buffer, sizeof(buffer), NULL);
		return buffer;
	}
	template <typename T>
	T ReadString(DWORD64 address) {
		T buffer;

		for (int i = 0; i < 256; i++) {
			char x = ReadMemory<char>(address + i);
			if (x == '\0') break;
			buffer += x;
		}
		return buffer;
	}

	template <typename T>
	void WriteMemory(DWORD64 address, T val) {

		WriteProcessMemory(processHandle, (LPVOID)address, &val, sizeof(val), NULL);
	}
};


int main()
{
	Memory mem;
	while (true)
	{
		DWORD64 localPlayer = mem.ReadMemory<DWORD64>(mem.client + offsets::dwLocalPlayer);

		if (!localPlayer) continue;

		DWORD localTeam = mem.ReadMemory<DWORD64>(localPlayer + offsets::m_iTeamNum);

		DWORD64 entityList = mem.ReadMemory<uintptr_t>(mem.client + offsets::dwEntityList);

		for (auto i = 1; i < 32; ++i)
		{
			DWORD64 listEntry = mem.ReadMemory<DWORD64>(entityList + (8 * (i & 0x7FFF) >> 9) + 16);
			if (!listEntry) continue;

			DWORD64 CurrentBasePlayerController = mem.ReadMemory<DWORD64>(listEntry + i * 0x78);


			DWORD64 m_hPlayerPawn = mem.ReadMemory<DWORD64>(CurrentBasePlayerController + offsets::m_hPlayerPawn);
			DWORD64 listEntry2 = mem.ReadMemory<DWORD64>(entityList + 0x8 * ((m_hPlayerPawn & 0x7FFF) >> 9) + 16);
			if (!listEntry2) continue;


			DWORD64 entity = mem.ReadMemory<DWORD64>(listEntry2 + 120 * (m_hPlayerPawn & 0x1FF));


			if (localPlayer == entity || entity == 0) continue;


			DWORD teamID = mem.ReadMemory<BYTE>(entity + offsets::m_iTeamNum);

			if (teamID != 2 && teamID != 3) continue;

			if (teamID == localTeam) continue;

			mem.WriteMemory<float>(entity + offsets::m_flDetectedByEnemySensorTime, 86400.f);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	return 0;
}