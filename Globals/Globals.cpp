#include "Globals.h"

HWND* g_pMainWnd;

char* stristr(const char* str1, const char* str2)
{
	const char* p1 = str1;
	const char* p2 = str2;
	const char* r = *p2 == 0 ? str1 : 0;

	while (*p1 != 0 && *p2 != 0)
	{
		if (tolower((unsigned char)*p1) == tolower((unsigned char)*p2))
		{
			if (r == 0)
			{
				r = p1;
			}

			p2++;
		}
		else
		{
			p2 = str2;
			if (r != 0)
			{
				p1 = r + 1;
			}

			if (tolower((unsigned char)*p1) == tolower((unsigned char)*p2))
			{
				r = p1;
				p2++;
			}
			else
			{
				r = 0;
			}
		}

		p1++;
	}

	return *p2 == 0 ? (char*)r : 0;
}

namespace GlobalOffsetDumper
{
	HANDLE g_Mutex = NULL;

	bool g_MmSelectProcess = false;

	bool g_MmAbout = false;
	bool g_MmExit = false;
	bool g_MmDestroy = false;

	CHAR g_SelectProcessFilter[MAX_PATH]{};
	CHAR g_InputClassNameBuffer[MAX_PATH]{};
	std::vector<DumpClassInfo> g_Classes;

	size_t GetSizeOfType(const DumpOffsetInfo* DumpOff)
	{
		SIZE_T Size = 0;
		const char* TypeC = DumpOff->OffsetType;

		if (stristr(TypeC, "*"))
		{
			Size = sizeof(PVOID);
		}
		else if (stristr(TypeC, "int"))
		{
			if (stristr(TypeC, "64"))
				Size = sizeof(__int64);
			else
				Size = sizeof(__int32);
		}
		else if (stristr(TypeC, "float"))
		{
			Size = sizeof(float);
		}
		else if (stristr(TypeC, "double"))
		{
			Size = sizeof(double);
		}
		else if (stristr(TypeC, "short"))
		{
			Size = sizeof(short);
		}
		else if (stristr(TypeC, "wchar"))
		{
			Size = sizeof(wchar_t);
		}
		else if (stristr(TypeC, "char"))
		{
			Size = sizeof(char);
		}


		if (char* arbeg = stristr(DumpOff->OffsetName, "["))
		{
			arbeg++;

			uintptr_t start = (uintptr_t)arbeg;
			uintptr_t end = 0;
			while (*arbeg != ']')
				arbeg++;
			end = (uintptr_t)arbeg;

			std::string sizeofarr = (char*)start;
			sizeofarr.resize(end - start);
			Size *= atoi(sizeofarr.c_str());
		}

		return Size;
	}

	bool LoadConfig()
	{
		OPENFILENAME ofn;
		ZeroMemory(&ofn, sizeof(ofn));

		wchar_t szFile[MAX_PATH];

		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = *g_pMainWnd;
		ofn.lpstrFile = szFile;
		// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
		// use the contents of szFile to initialize itself.
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = L"Global Offset Dumper (.GLODCFG)\0*.glodcfg\0";
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		if (GetOpenFileName(&ofn))
		{
			HANDLE FileHandle = CreateFile(ofn.lpstrFile, GENERIC_READ, 0, (LPSECURITY_ATTRIBUTES)NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
			if (FileHandle == INVALID_HANDLE_VALUE)
				return false;

			DWORD FileSize = GetFileSize(FileHandle, NULL);
			if (FileSize == INVALID_FILE_SIZE)
			{
				CloseHandle(FileHandle);
				return false;
			}

			DWORD CopyFileSize = FileSize;
			CHAR* FileBuffer = new CHAR[FileSize];
			DWORD BytesRead = 0;
			do
			{
				CopyFileSize -= BytesRead;

				if (!CopyFileSize)
					break;
			} while (ReadFile(FileHandle, FileBuffer, CopyFileSize > 1000 ? 1000 : CopyFileSize, &BytesRead, NULL));

			CloseAllMenus();		// To prevent crashes
			ClearAllProcesses();	// To load the config freshly

			g_Classes.clear();		// To prevent configs loading on top of each other

			PCHAR OffsetStart = nullptr;
			BYTE ItemNumber = 0;

			DumpClassInfo DumpClass;

			DumpOffsetInfo DumpOffset;
			for (PCHAR Idx = FileBuffer; Idx < FileBuffer + FileSize; Idx++)
			{
				if (*Idx == '\n')
				{
					ItemNumber = 0;
					g_Classes.push_back(DumpClass);

					DumpClass.Offsets.clear();
					continue;
				}

				switch (*Idx)
				{
					case '@':
					{
						OffsetStart = Idx + 1;
						break;
					}
					case '#':
					{
						if (!OffsetStart)
							assert(FALSE);

						SIZE_T Limit = Idx - OffsetStart;
						std::string Item = (char*)OffsetStart;
						Item.resize(Limit);

						switch (ItemNumber)
						{
							case 0:
							{
								DumpClass.ClassName = Item.c_str();
								ItemNumber++;
								break;
							}
							case 1:
							{
								DumpOffset.SelectedModule = atoi(Item.c_str());
								ItemNumber++;
								break;
							}
							case 2:
							{
								DumpOffset.SelectedSize = (BYTESIZE)atoi(Item.c_str());
								ItemNumber++;
								break;
							}
							case 3:
							{
								strcpy_s(DumpOffset.OffsetType, MAX_PATH, Item.c_str());
								ItemNumber++;
								break;
							}
							case 4:
							{
								strcpy_s(DumpOffset.OffsetSize, MAX_PATH, Item.c_str());
								ItemNumber++;
								break;
							}
							case 5:
							{
								strcpy_s(DumpOffset.OffsetName, MAX_PATH, Item.c_str());
								ItemNumber++;
								break;
							}
							case 6:
							{
								strcpy_s(DumpOffset.Signature, MAX_PATH, Item.c_str());
								ItemNumber = 1;
								DumpClass.Offsets.push_back(DumpOffset);
								break;
							}
						}
						break;
					}
				}
			}

			delete[] FileBuffer;
			CloseHandle(FileHandle);

			return true;
		}

		return false;
	}

	bool SaveConfig()
	{
		OPENFILENAME ofn;
		ZeroMemory(&ofn, sizeof(ofn));

		wchar_t szFile[MAX_PATH];

		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = *g_pMainWnd;
		ofn.lpstrFile = szFile;
		// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
		// use the contents of szFile to initialize itself.
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = L"Global Offset Dumper (.GLODCFG)\0*.glodcfg\0";
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

		if (GetSaveFileName(&ofn))
		{
			HANDLE FileHandle = CreateFile(ofn.lpstrFile, GENERIC_READ | GENERIC_WRITE, 0, (LPSECURITY_ATTRIBUTES)NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
			if (FileHandle == INVALID_HANDLE_VALUE)
				return false;

			// BEST CONFIG SYSTEM EVER
			DWORD BytesWritten = 0;
			for (auto& klass : g_Classes)
			{
				std::string Buffer = "CLASS NAME: @";
				Buffer.append(klass.ClassName);
				Buffer.append("# ");
				for (auto& offset : klass.Offsets)
				{
					Buffer.append("MODULE ID: @");
					Buffer.append(std::to_string(offset.SelectedModule));
					Buffer.append("# MEMORY TYPE: @");
					Buffer.append(std::to_string((int)offset.SelectedSize));
					Buffer.append("# TYPE: @");
					Buffer.append(offset.OffsetType);
					Buffer.append("# TYPE SIZE: @");
					Buffer.append(offset.OffsetSize);
					Buffer.append("# OFFSET NAME: @");
					Buffer.append(offset.OffsetName);
					Buffer.append("# SIGNATURE: @");
					Buffer.append(offset.Signature);
					Buffer.append("# ");
				}
				Buffer.append("\n");
				WriteFile(FileHandle, Buffer.c_str(), (DWORD)Buffer.size(), &BytesWritten, NULL);
			}

			CloseHandle(FileHandle);

			return true;
		}

		return false;
	}

	bool DumpOffsetsHeader()
	{
		OPENFILENAME ofn;
		ZeroMemory(&ofn, sizeof(ofn));

		wchar_t szFile[MAX_PATH];

		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = *g_pMainWnd;
		ofn.lpstrFile = szFile;
		// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
		// use the contents of szFile to initialize itself.
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = L"C++ Header Files (.h)\0*.h\0";
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

		if (GetSaveFileName(&ofn))
		{
			HANDLE FileHandle = CreateFile(ofn.lpstrFile, GENERIC_READ | GENERIC_WRITE, 0, (LPSECURITY_ATTRIBUTES)NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
			if (FileHandle == INVALID_HANDLE_VALUE)
				return false;

			// BEST HEADER SYSTEM EVER
			DWORD BytesWritten = 0;

			for (int idx = 0; idx < g_Classes.size(); idx++)
			{
				auto& klass = g_Classes.at(idx);

				std::sort(klass.Offsets.begin(), klass.Offsets.end(), [](const DumpOffsetInfo& off1, const DumpOffsetInfo& off2) -> bool {
					return off1.Offset < off2.Offset;
					});

				std::string Buffer;
				if (idx == 0)
					Buffer.append("// Created by Global Offset Dumper\n// https://github.com/paskalian/Global-Offset-Dumper\n\n\n");

				Buffer.append("struct ");
				Buffer.append(klass.ClassName);
				Buffer.append("\n{\n");

				size_t PadIdx = 0;
				for (size_t OffsetIdx = 0; OffsetIdx < klass.Offsets.size(); OffsetIdx++)
				{
					const auto& offset = klass.Offsets.at(OffsetIdx);

					uintptr_t Offset = 0;

					if (OffsetIdx == 0)
					{
						Offset = offset.Offset;
					}
					else
					{
						const auto& prevOffset = klass.Offsets.at(OffsetIdx - 1);
						Offset = offset.Offset - (prevOffset.Offset + atoi(prevOffset.OffsetSize));
					}

					if (Offset)
					{
						Buffer += "char pad" + std::to_string(PadIdx) + "[" + std::to_string(Offset) + "];\n";
						PadIdx++;
					}

					Buffer.append(offset.OffsetType);
					Buffer.append(" ");
					Buffer.append(offset.OffsetName);
					Buffer.append(";\n");
				}

				Buffer.append("};\n\n");
				WriteFile(FileHandle, Buffer.c_str(), (DWORD)Buffer.size(), &BytesWritten, NULL);
			}

			CloseHandle(FileHandle);

			return true;
		}

		return false;
	}
}