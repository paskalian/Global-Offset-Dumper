#include "Renderers.h"

#include <Windows.h>

void GlobalOffsetDumper::CloseAllMenus()
{
    g_MmSelectProcess = FALSE;
    g_MmAbout = FALSE;
    g_MmExit = FALSE;
}

void GlobalOffsetDumper::Render()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos, true, ImVec2(0, 0));
    ImGui::SetNextWindowSize(viewport->Size, true);

    ImGui::Begin("Main Area", NULL, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Main"))
        {
            if (ImGui::MenuItem("Select Process"))
                g_MmSelectProcess = true;
            if (ImGui::MenuItem("Load Configuration"))
            {
                if (g_SelectedProcess.PID)
                {
                    HANDLE ThreadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)GlobalOffsetDumper::LoadConfig, NULL, NULL, NULL);
                    if (ThreadHandle && ThreadHandle != INVALID_HANDLE_VALUE)
                        CloseHandle(ThreadHandle);
                }
            }
            if (ImGui::MenuItem("Save Configuration"))
            {
                if (g_SelectedProcess.PID)
                {
                    HANDLE ThreadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)GlobalOffsetDumper::SaveConfig, NULL, NULL, NULL);
                    if (ThreadHandle && ThreadHandle != INVALID_HANDLE_VALUE)
                        CloseHandle(ThreadHandle);
                }
            }
            if (ImGui::MenuItem("About"))
                g_MmAbout = true;
            if (ImGui::MenuItem("Exit"))
                g_MmExit = true;

            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    RenderConfiguration();

    ImGui::End();

    static bool OldMmSelectProcess = false;

    if (OldMmSelectProcess != g_MmSelectProcess && g_MmSelectProcess)
    {
        
        HANDLE ThreadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)GetAllProcesses, NULL, NULL, NULL);
        if (ThreadHandle && ThreadHandle != INVALID_HANDLE_VALUE)
            CloseHandle(ThreadHandle);
        

        /*GetAllProcesses();*/
    }
    OldMmSelectProcess = g_MmSelectProcess;
    

    if (GlobalOffsetDumper::g_MmSelectProcess)
        RenderSelectProcess();

    if (GlobalOffsetDumper::g_MmAbout)
        RenderAbout();

    if (GlobalOffsetDumper::g_MmExit)
        RenderExit();
}

void GlobalOffsetDumper::RenderSelectProcess()
{
    ImGui::Begin("Select Process", &g_MmSelectProcess, ImGuiWindowFlags_NoCollapse);

    ImGui::Text("PROCESSES\n");
    if (ImGui::BeginListBox(" "))
    {
        for (unsigned int idx = 0; idx < g_Processes.size(); idx++)
        {
            ProcessInfo& IdxProcess = g_Processes[idx];
            if (!IdxProcess.ProcessName.empty())
            {
                if (ImGui::Selectable(IdxProcess.ProcessName.c_str(), &IdxProcess.IsSelected))
                    g_DummySelectProcess = &IdxProcess;
            }
        }

        ImGui::EndListBox();
    }

    if (g_DummySelectProcess)
    {
        ImGui::SameLine();
        ImGui::Text("%s\n\nBASE: 0x%X\nSIZE: 0x%X\nPARENT PID: %i\nPID: %i\nTHREADS: %i\n", g_DummySelectProcess->ProcessName.c_str(), g_DummySelectProcess->ModInfos.at(0).BaseAddress, g_DummySelectProcess->ModInfos.at(0).BaseSize, g_DummySelectProcess->ParentPID, g_DummySelectProcess->PID, g_DummySelectProcess->ThreadCount);
    }

    if (ImGui::InputText("Filter", g_SelectProcessFilter, MAX_PATH, ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank, [](ImGuiInputTextCallbackData* data) -> int {
        ImWchar c = data->EventChar;
    return !((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c == '_'));
        }))
    {
        HANDLE ThreadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)GetAllProcesses, NULL, NULL, NULL);
        if (ThreadHandle && ThreadHandle != INVALID_HANDLE_VALUE)
            CloseHandle(ThreadHandle);
    }

    if (ImGui::Button("Renew"))
    {
        HANDLE ThreadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)GetAllProcesses, NULL, NULL, NULL);
        if (ThreadHandle && ThreadHandle != INVALID_HANDLE_VALUE)
            CloseHandle(ThreadHandle);
    }
    ImGui::SameLine();
    if (ImGui::Button("Select"))
    {
        if (g_DummySelectProcess)
        {
            // Cleanup for changing procedure
            if (g_SelectedProcess.ProcHandle && g_SelectedProcess.ProcHandle != INVALID_HANDLE_VALUE)
                CloseHandle(g_SelectedProcess.ProcHandle);

            // Memory cleanup & new init
            g_SelectedProcess = *g_DummySelectProcess;
            
            for (auto& klass : g_Classes)
                for (auto& offset : klass.Offsets)
                    offset.SelectedModule = 0;

            ClearAllProcesses();

            HANDLE ThreadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)GetHandle, (LPVOID)HANDLETYPE::OpenProcess, NULL, NULL);
            if (ThreadHandle && ThreadHandle != INVALID_HANDLE_VALUE)
                CloseHandle(ThreadHandle);

            g_MmSelectProcess = false;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
        ClearAllProcesses();

        g_MmSelectProcess = false;
    }
    ImGui::End();
}

void GlobalOffsetDumper::RenderAbout()
{
    ImGui::Begin("About", &g_MmAbout, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    ImGui::Text("Global Offset Dumper 1.0\nCR 2023 | All Rights Reserved");
    ImGui::End();
}

void GlobalOffsetDumper::RenderExit()
{
    ImGui::Begin("Sure?", &g_MmExit, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    g_MmDestroy = ImGui::Button("YES I AM SURE");
    ImGui::End();
}

void GlobalOffsetDumper::RenderConfiguration()
{ 
    float TextHeight = ImGui::GetTextLineHeight();

    ImGui::Text("Selected Process: %s (HANDLE: 0x%X) | Base Address: 0x%X", g_SelectedProcess.PID ? g_SelectedProcess.ProcessName.c_str() : "NULL", g_SelectedProcess.PID ? g_SelectedProcess.ProcHandle : 0, g_SelectedProcess.PID ? g_SelectedProcess.ModInfos.at(0).BaseAddress : 0);
   
    if (g_SelectedProcess.PID)
    {
        if (ImGui::InputText("Class Name", g_InputClassNameBuffer, MAX_PATH, ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank, [](ImGuiInputTextCallbackData* data) -> int {
            ImWchar c = data->EventChar;
            return !((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c == '_'));
        }))
        {
            bool AlreadyThere = false;
            for (auto gdclass : g_Classes)
            {
                if (strcmp(gdclass.ClassName.c_str(), g_InputClassNameBuffer) == 0)
                {
                    AlreadyThere = true;
                    break;
                }
            }

            if (!AlreadyThere)
            {
                DumpClassInfo DumpClass;
                DumpClass.ClassName = g_InputClassNameBuffer;
                GlobalOffsetDumper::g_Classes.push_back(DumpClass);
            }
        }
    
        for (SIZE_T idx = 0; idx < g_Classes.size(); idx++)
        {
            DumpClassInfo* IdxClass = &g_Classes.at(idx);

            if (ImGui::TreeNodeEx(IdxClass->ClassName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::Button("Add"))
                {
                    DumpOffsetInfo OffsetInfo;
                    IdxClass->Offsets.push_back(OffsetInfo);
                }
                ImGui::SameLine();
                if (ImGui::Button("Del"))
                {
                    if (IdxClass->Offsets.size() == 0)
                        g_Classes.erase(g_Classes.begin() + idx, g_Classes.begin() + idx + 1);
                    else
                        IdxClass->Offsets.erase(IdxClass->Offsets.end() - 1, IdxClass->Offsets.end());
                }

                for (SIZE_T idx2 = 0; idx2 < IdxClass->Offsets.size(); idx2++)
                {
                    static const char* Sizes[] = { "BYTE", "WORD", "DWORD"/*, "QWORD"*/ };

                    DumpOffsetInfo* IdxOffset = &IdxClass->Offsets.at(idx2);

                    std::string ModuleSearchName = "Module##";
                    ModuleSearchName.append(std::to_string(idx2));

                    ImGui::SetNextItemWidth(TextHeight * 6);
                    if (ImGui::BeginCombo(ModuleSearchName.c_str(), g_SelectedProcess.ModInfos.at(IdxOffset->SelectedModule).ModuleName.c_str()))
                    {
                        for (SIZE_T ModIdx = 0; ModIdx < g_SelectedProcess.ModInfos.size(); ModIdx++)
                        {
                            ModuleInfo& IdxMod = g_SelectedProcess.ModInfos.at(ModIdx);
                            if (ImGui::Selectable(IdxMod.ModuleName.c_str(), &g_SelectedProcess.IsSelected))
                                IdxOffset->SelectedModule = (int)ModIdx;
                        }

                        ImGui::EndCombo();
                    }
                    ImGui::SameLine();

                    std::string OffsetSizeName = "Memory##";
                    OffsetSizeName.append(std::to_string(idx2));

                    ImGui::SetNextItemWidth(TextHeight * 6);
                    ImGui::Combo(OffsetSizeName.c_str(), (int*)&IdxOffset->SelectedSize, Sizes, ARRAYSIZE(Sizes));
                    ImGui::SameLine();

                    std::string OffsetTypeName = "##";
                    OffsetTypeName.append(std::to_string(idx2));

                    ImGui::SetNextItemWidth(TextHeight * 10);
                    if (ImGui::InputText(OffsetTypeName.c_str(), IdxOffset->OffsetType, MAX_PATH, ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank, [](ImGuiInputTextCallbackData* data) -> int {
                        ImWchar c = data->EventChar;
                    return !((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c == '_') || (c == '*') || (c == '&'));
                        }))
                    {
                        std::string IdxOffsetType = std::to_string(GetSizeOfType(IdxOffset));

                        strcpy_s(IdxOffset->OffsetSize, MAX_PATH, IdxOffsetType.c_str());
                    }

                    ImGui::SameLine();

                    std::string OffsetTypeSizeName = "Type##";
                    OffsetTypeSizeName.append(std::to_string(idx2));

                    ImGui::SetNextItemWidth(TextHeight * 2);
                    ImGui::InputText(OffsetTypeSizeName.c_str(), IdxOffset->OffsetSize, MAX_PATH, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank, [](ImGuiInputTextCallbackData* data) -> int {
                        ImWchar c = data->EventChar;
                    return !((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c == '_') || (c == '*') || (c == '&'));
                        });
                   
                    ImGui::SameLine();

                    std::string OffsetInpName = "Name##";
                    OffsetInpName.append(std::to_string(idx2));

                    ImGui::SetNextItemWidth(TextHeight * 10);
                    ImGui::InputText(OffsetInpName.c_str(), IdxOffset->OffsetName, MAX_PATH, ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank, [](ImGuiInputTextCallbackData* data) -> int {
                        ImWchar c = data->EventChar;
                    return !((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c == '_') || (c == '[') || (c == ']'));
                        });
                    ImGui::SameLine();

                    std::string SignatureInpName = "Signature##";
                    SignatureInpName.append(std::to_string(idx2));

                    ImGui::SetNextItemWidth(TextHeight * 10);
                    ImGui::InputText(SignatureInpName.c_str(), IdxOffset->Signature, MAX_PATH, ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_CharsUppercase, [](ImGuiInputTextCallbackData* data) -> int {
                        ImWchar c = data->EventChar;
                    return !((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') || (c == ' ') || (c == '?'));
                        });
                    ImGui::SameLine();

                    ImGui::Text("OFFSET: 0x%X", IdxOffset->Offset);
                }
                ImGui::TreePop();
            }
        }
    }

    if (ImGui::Button("D U M P", ImVec2(ImGui::GetWindowWidth(), TextHeight * 3)))
    {
        HANDLE ThreadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)DumpOffsets, NULL, NULL, NULL);
        if (ThreadHandle && ThreadHandle != INVALID_HANDLE_VALUE)
            CloseHandle(ThreadHandle);
    }
    
    std::string FpsText = std::to_string((int)floor(ImGui::GetIO().Framerate));
    FpsText.append(" FPS");
    ImGui::Text(FpsText.c_str());
}