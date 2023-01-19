#pragma once

#include "..\Globals\Globals.h"

enum class HANDLETYPE
{
	OpenProcess,
	//NtOpenProcess,
	//HandleHijacking,
	//ZwOpenProcess,
	//DKOM
};

namespace GlobalOffsetDumper
{
	std::vector<BYTE> SigToActualSig(const char* Sig);
	PVOID SigScan(PCHAR StartAddress, SIZE_T Size, std::vector<BYTE> Pattern);
	VOID DumpOffsets();
	VOID GetHandle(HANDLETYPE HandleType);
}