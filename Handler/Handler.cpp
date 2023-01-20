#include "Handler.h"

namespace GlobalOffsetDumper
{
	std::vector<BYTE> SigToActualSig(const char* Sig)
	{
		SIZE_T SigLen = strlen(Sig);

		std::vector<BYTE> ActualSig;
		for (const char* c = Sig; c < Sig + SigLen + 1; c += 3)
			ActualSig.push_back((BYTE)strtol(c, NULL, 16));

		return ActualSig;
	}

	PVOID SigScan(PCHAR StartAddress, SIZE_T Size, std::vector<BYTE> Pattern)
	{
		SIZE_T PatternLen = Pattern.size();

		bool Found = TRUE;
		for (unsigned int i1 = 0; i1 < Size; i1++)
		{
			Found = TRUE;
			for (unsigned int i2 = 0; i2 < PatternLen; i2++)
			{
				if (Pattern[i2] != '\x00' && StartAddress[i1 + i2] != Pattern[i2])
				{
					Found = FALSE;
					break;
				}
			}

			if (Found)
				return StartAddress + i1 + PatternLen;
		}

		return nullptr;
	}

	PVOID SigScanEx(HANDLE ProcessHandle, PCHAR StartAddress, SIZE_T Size, std::vector<BYTE> Pattern)
	{
		SIZE_T PatternLen = Pattern.size();

		const int PAGE_SIZE = 0x1000;

		BYTE Data[PAGE_SIZE]{};
		SIZE_T ReadBytes = 0;
		SIZE_T TotalReadBytes = 0;

		while (Size)
		{
			assert(ReadProcessMemory(ProcessHandle, StartAddress + TotalReadBytes, &Data, PAGE_SIZE, &ReadBytes));

			Size = Size >= ReadBytes ? Size - ReadBytes : 0;

			assert(ReadBytes == PAGE_SIZE);

			bool Found = TRUE;
			for (unsigned int i1 = 0; i1 < PAGE_SIZE; i1++)
			{
				Found = TRUE;
				for (unsigned int i2 = 0; i2 < PatternLen; i2++)
				{
					if (Pattern[i2] != '\x00' && Data[i1 + i2] != Pattern[i2])
					{
						Found = FALSE;
						break;
					}
				}

				if (Found)
					return StartAddress + TotalReadBytes + i1 + PatternLen;
			}

			TotalReadBytes += ReadBytes;
		}

		return nullptr;
	}

	VOID DumpOffsets()
	{
		for (const auto& klass : g_Classes)
		{
			for (const auto& offset : klass.Offsets)
			{
				ModuleInfo& IdxModule = g_SelectedProcess.ModInfos.at(offset.SelectedModule);
				PVOID Address = SigScanEx(g_SelectedProcess.ProcHandle, (PCHAR)IdxModule.BaseAddress, IdxModule.BaseSize, SigToActualSig(offset.Signature));
				
				SIZE_T ReadSize = 4;

				switch (offset.SelectedSize)
				{
				case BYTESIZE::BYTE:
				{
					ReadSize = sizeof(BYTE);
					break;
				}
				case BYTESIZE::WORD:
				{
					ReadSize = sizeof(WORD);
					break;
				}
				case BYTESIZE::DWORD:
				{
					ReadSize = sizeof(DWORD);
					break;
				}
				/*	case BYTESIZE::QWORD:
				{
					ReadSize = sizeof(DWORD) * 2;
					break;
				}*/
				}
				
				ZeroMemory((PVOID)&offset.Offset, sizeof(offset.Offset));
				ReadProcessMemory(g_SelectedProcess.ProcHandle, Address, (LPVOID)&offset.Offset, ReadSize, nullptr);
			}
		}

		

		DumpOffsetsHeader();
	}

	VOID GetHandle(HANDLETYPE HandleType)
	{
		switch (HandleType)
		{
		case HANDLETYPE::OpenProcess:
		{
			GlobalOffsetDumper::g_SelectedProcess.ProcHandle = OpenProcess(PROCESS_VM_READ, FALSE, GlobalOffsetDumper::g_SelectedProcess.PID);
			break;
		}
		}
	}
}