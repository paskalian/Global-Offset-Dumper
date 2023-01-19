#include "Process.h"

namespace GlobalOffsetDumper
{
	std::vector<ProcessInfo> g_Processes;
	ProcessInfo* g_DummySelectProcess = nullptr;
	ProcessInfo g_SelectedProcess{};

	void ClearAllProcesses()
	{
		g_DummySelectProcess = nullptr;

		for (ProcessInfo& proc : g_Processes)
			proc.ModInfos.clear();
		g_Processes.clear();
	}

	void GetAllProcesses()
	{
		ClearAllProcesses();

		PROCESSENTRY32 procEntry;
		procEntry.dwSize = sizeof(PROCESSENTRY32);

		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		if (hSnapshot == INVALID_HANDLE_VALUE)
			return;

		Process32First(hSnapshot, &procEntry);
		do
		{
			char ProcessNameBuffer[MAX_PATH]{};
			size_t ReturnBytes = 0;
#ifdef _WIN64
			wcstombs_s(&ReturnBytes, ProcessNameBuffer, procEntry.szExeFile, wcslen(procEntry.szExeFile));
#else
			wcstombs_s(&ReturnBytes, ProcessNameBuffer, MAX_PATH, szExeFile, wcslen(procEntry.szExeFile));
#endif

			if (wcsstr(procEntry.szExeFile, L".exe") && stristr(ProcessNameBuffer, g_SelectProcessFilter))
			{
				ProcessInfo ProcInfo{};

				PWCHAR ProcessName = procEntry.szExeFile;
				ProcInfo.ProcessNameLimit = (BYTE)wcslen(ProcessName);

				std::wstring ProcessFinalName = ProcessName;

				ProcessFinalName.append(L" (PID: ");
				ProcessFinalName.append(std::to_wstring(procEntry.th32ProcessID));
				ProcessFinalName.append(L")");

				char ProcessFinalNameBuffer[MAX_PATH]{};
				size_t ReturnBytes = 0;
#ifdef _WIN64
				wcstombs_s(&ReturnBytes, ProcessFinalNameBuffer, ProcessFinalName.c_str(), ProcessFinalName.length());
#else
				wcstombs_s(&ReturnBytes, ProcessFinalNameBuffer, MAX_PATH, ProcessFinalName.c_str(), ProcessFinalName.length());
#endif

				ProcInfo.ProcessName = ProcessFinalNameBuffer;
				ProcInfo.ParentPID = procEntry.th32ParentProcessID;
				ProcInfo.PID = procEntry.th32ProcessID;
				ProcInfo.ThreadCount = procEntry.cntThreads;

				GetModInfo(&ProcInfo);

				if (!ProcInfo.ModInfos.size() || !ProcInfo.ModInfos.at(0).BaseAddress)
					continue;

				g_Processes.push_back(ProcInfo);
			}
		} while (Process32Next(hSnapshot, &procEntry));

		std::sort(g_Processes.begin(), g_Processes.end(), [](const ProcessInfo& struct1, const ProcessInfo& struct2) {
			return (struct1.PID < struct2.PID);
			}
		);
		
		CloseHandle(hSnapshot);
	}

	VOID GetModInfo(ProcessInfo* ProcInfo)
	{
		MODULEENTRY32 ModEntry;
		ZeroMemory(&ModEntry, sizeof(MODULEENTRY32));

		ModEntry.dwSize = sizeof(MODULEENTRY32);

		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ProcInfo->PID);
		if (hSnapshot == INVALID_HANDLE_VALUE)
			return;

		ProcInfo->ModInfos.clear();

		Module32First(hSnapshot, &ModEntry);
		do
		{
			CHAR Buffer[MAX_PATH]{};
			size_t StrLen = wcslen(ModEntry.szModule) + 1;

			size_t ReturnBytes = 0;

#ifdef _WIN64
			wcstombs_s(&ReturnBytes, Buffer, ModEntry.szModule, StrLen);
#else
			wcstombs_s(&ReturnBytes, Buffer, MAX_PATH, ModEntry.szModule, StrLen);
#endif

			ModuleInfo ModInfo{};
			ModInfo.hModule = ModEntry.hModule;
			ModInfo.ModuleName = Buffer;
			ModInfo.BaseAddress = (uintptr_t)ModEntry.modBaseAddr;
			ModInfo.BaseSize = ModEntry.modBaseSize;

			ProcInfo->ModInfos.push_back(ModInfo);
		} while (Module32Next(hSnapshot, &ModEntry));

		CloseHandle(hSnapshot);
	}
}