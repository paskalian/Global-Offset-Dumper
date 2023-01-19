#pragma once

#include <vector>
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <Psapi.h>
#include <algorithm>
#include "..\ImGui\imgui.h"
#include "..\Process\Process.h"
#include "..\Renderers\Renderers.h"

extern HWND* g_pMainWnd;

char* stristr(const char* str1, const char* str2);

namespace GlobalOffsetDumper
{
	extern HANDLE g_Mutex;

	enum class BYTESIZE
	{
		BYTE,
		WORD,
		DWORD
	};

	struct DumpOffsetInfo
	{
		CHAR OffsetName[MAX_PATH]{};
		CHAR OffsetType[MAX_PATH]{};
		CHAR OffsetSize[MAX_PATH]{};
		CHAR Signature[MAX_PATH]{};
		BYTESIZE SelectedSize = BYTESIZE::BYTE;
		int SelectedModule = 0;
		uintptr_t Offset = 0;
	};

	struct DumpClassInfo
	{
		std::string ClassName{};
		std::vector<DumpOffsetInfo> Offsets;
	};

	extern bool g_MmSelectProcess;
	
	extern bool g_MmAbout;
	extern bool g_MmExit;
	extern bool g_MmDestroy;

	extern CHAR g_SelectProcessFilter[MAX_PATH];
	extern CHAR g_InputClassNameBuffer[MAX_PATH];
	extern std::vector<DumpClassInfo> g_Classes;

	size_t GetSizeOfType(const DumpOffsetInfo* DumpOff);
	bool LoadConfig();
	bool SaveConfig();
	bool DumpOffsetsHeader();
}