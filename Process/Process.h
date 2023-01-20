#pragma once

#include "..\Globals\Globals.h"

namespace GlobalOffsetDumper
{
	struct ModuleInfo
	{
		HMODULE hModule;
		std::string ModuleName;
		uintptr_t BaseAddress;
		uintptr_t BaseSize;
	};

	struct ProcessInfo
	{
		~ProcessInfo()
		{
			if (this->ProcHandle && this->ProcHandle != INVALID_HANDLE_VALUE)
				CloseHandle(ProcHandle);
		}

		HANDLE ProcHandle;

		std::string ProcessName;
		BYTE ProcessNameLimit;
		ULONG ParentPID;
		ULONG PID;
		ULONG ThreadCount;
		std::vector<ModuleInfo> ModInfos;

		BOOL IsSelected;
	};

	extern std::vector<ProcessInfo> g_Processes;
	extern ProcessInfo* g_DummySelectProcess;
	extern ProcessInfo g_SelectedProcess;

	void ClearAllProcesses();
	void GetAllProcesses();
	VOID GetModInfo(ProcessInfo* ProcInfo);
}