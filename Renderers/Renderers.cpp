#include "Renderers.h"

#include <Windows.h>

void GlobalOffsetDumper::CloseAllMenus()
{
    g_MmSelectProcess = FALSE;
    g_MmAbout = FALSE;
    g_MmExit = FALSE;
}

constexpr ULONG IMGUI_MAINAREA_STYLE    = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus;
constexpr ULONG IMGUI_NORMAL_STYLE      = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
constexpr ULONG IMGUI_INPUTTEXT_STYLE   = ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue;

void GlobalOffsetDumper::Render()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos, true, ImVec2(0, 0));
    ImGui::SetNextWindowSize(viewport->Size, true);

    ImGui::Begin("Main Area", NULL, IMGUI_MAINAREA_STYLE);

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
                    CreateGlodThread((LPTHREAD_START_ROUTINE)LoadConfig);
                }
                else
                {
                    MessageBoxA(*g_pMainWnd, "Select a process first", "Global Offset Dumper", MB_OK);
                }
            }
            if (ImGui::MenuItem("Save Configuration"))
            {
                if (g_SelectedProcess.PID)
                {
                    CreateGlodThread((LPTHREAD_START_ROUTINE)SaveConfig);
                }
                else
                {
                    MessageBoxA(*g_pMainWnd, "Select a process first", "Global Offset Dumper", MB_OK);
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
        CreateGlodThread((LPTHREAD_START_ROUTINE)GetAllProcesses);
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
    if (ImGui::BeginListBox("##Processes"))
    {
        for (auto& IdxProcess : g_Processes)
        {
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
        ImGui::Text("%s\n\nBase: 0x%X\nBase Size: 0x%X\nParent Pid: %i\nPid: %i\nThreads: %i\n", g_DummySelectProcess->ProcessName.c_str(), g_DummySelectProcess->ModInfos.at(0).BaseAddress, g_DummySelectProcess->ModInfos.at(0).BaseSize, g_DummySelectProcess->ParentPID, g_DummySelectProcess->PID, g_DummySelectProcess->ThreadCount);
    }

    if (ImGui::InputText("Filter", g_SelectProcessFilter, MAX_PATH, IMGUI_INPUTTEXT_STYLE, [](ImGuiInputTextCallbackData* data) -> int {
        ImWchar c = data->EventChar;
        return !((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c == '_'));
    }))
    {
        CreateGlodThread((LPTHREAD_START_ROUTINE)GetAllProcesses);
    }

    if (ImGui::Button("Renew"))
    {
        CreateGlodThread((LPTHREAD_START_ROUTINE)GetAllProcesses);
    }
    ImGui::SameLine();
    if (ImGui::Button("Select"))
    {
        if (g_DummySelectProcess)
        {
            // Cleanup for changing procedure
            if (IsHandleValid(g_SelectedProcess.ProcHandle))
                CloseHandle(g_SelectedProcess.ProcHandle);

            // Memory cleanup & new init
            g_SelectedProcess = *g_DummySelectProcess;
            
            for (auto& klass : g_Classes)
                for (auto& offset : klass.Offsets)
                    offset.SelectedModule = 0;

            ClearAllProcesses();

            CreateGlodThread((LPTHREAD_START_ROUTINE)GetHandle, (LPVOID)HANDLETYPE::OpenProcess);

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
    ImGui::Begin("About", &g_MmAbout, IMGUI_NORMAL_STYLE);
    ImGui::Text("Global Offset Dumper 1.0\nhttps://github.com/paskalian/Global-Offset-Dumper");
    ImGui::End();
}

void GlobalOffsetDumper::RenderExit()
{
    ImGui::Begin("Sure?", &g_MmExit, IMGUI_NORMAL_STYLE);
    g_MmDestroy = ImGui::Button("YES I AM SURE");
    ImGui::End();
}

void GlobalOffsetDumper::RenderConfiguration()
{ 
    ImGui::Text("Selected Process: %s (HANDLE: 0x%X) | Base Address: 0x%X", g_SelectedProcess.PID ? g_SelectedProcess.ProcessName.c_str() : "NULL", g_SelectedProcess.PID ? g_SelectedProcess.ProcHandle : 0, g_SelectedProcess.PID ? g_SelectedProcess.ModInfos.at(0).BaseAddress : 0);
    
    float TextHeight = ImGui::GetTextLineHeight();
    if (g_SelectedProcess.PID)
    {
        if (ImGui::InputText("Class Name", g_InputClassNameBuffer, MAX_PATH, IMGUI_INPUTTEXT_STYLE, [](ImGuiInputTextCallbackData* data) -> int {
            ImWchar c = data->EventChar;
            return !((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c == '_'));
        }))
        {
            bool AlreadyThere = false;
            for (auto IdxClass : g_Classes)
            {
                if (strcmp(IdxClass.ClassName.c_str(), g_InputClassNameBuffer) == 0)
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
    
        for (SIZE_T ClassIdx = 0; ClassIdx < g_Classes.size(); ClassIdx++)
        {
            DumpClassInfo* IdxClass = &g_Classes.at(ClassIdx);

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
                        g_Classes.erase(g_Classes.begin() + ClassIdx, g_Classes.begin() + ClassIdx + 1);
                    else
                        IdxClass->Offsets.erase(IdxClass->Offsets.end() - 1, IdxClass->Offsets.end());
                }

                for (SIZE_T OffsetIdx = 0; OffsetIdx < IdxClass->Offsets.size(); OffsetIdx++)
                {
                    static const char* Sizes[] = { "BYTE", "WORD", "DWORD" };

                    DumpOffsetInfo* IdxOffset = &IdxClass->Offsets.at(OffsetIdx);

                    std::string ModuleSearchName = "Module##";
                    ModuleSearchName.append(std::to_string(OffsetIdx));

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
                    OffsetSizeName.append(std::to_string(OffsetIdx));

                    ImGui::SetNextItemWidth(TextHeight * 6);
                    ImGui::Combo(OffsetSizeName.c_str(), (int*)&IdxOffset->SelectedSize, Sizes, ARRAYSIZE(Sizes));
                    ImGui::SameLine();

                    std::string OffsetTypeName = "##";
                    OffsetTypeName.append(std::to_string(OffsetIdx));

                    ImGui::SetNextItemWidth(TextHeight * 10);
                    if (ImGui::InputText(OffsetTypeName.c_str(), IdxOffset->OffsetType, MAX_PATH, IMGUI_INPUTTEXT_STYLE, [](ImGuiInputTextCallbackData* data) -> int {
                        ImWchar c = data->EventChar;
                        return !((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c == '_') || (c == '*') || (c == '&'));
                    }))
                    {
                        std::string IdxOffsetType = std::to_string(GetSizeOfType(IdxOffset));

                        strcpy_s(IdxOffset->OffsetSize, MAX_PATH, IdxOffsetType.c_str());
                    }
                    ImGui::SameLine();

                    std::string OffsetTypeSizeName = "Type##";
                    OffsetTypeSizeName.append(std::to_string(OffsetIdx));

                    ImGui::SetNextItemWidth(TextHeight * 2);
                    ImGui::InputText(OffsetTypeSizeName.c_str(), IdxOffset->OffsetSize, MAX_PATH, IMGUI_INPUTTEXT_STYLE, [](ImGuiInputTextCallbackData* data) -> int {
                        ImWchar c = data->EventChar;
                        return !((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c == '_') || (c == '*') || (c == '&'));
                    });
                    ImGui::SameLine();

                    std::string OffsetInpName = "Name##";
                    OffsetInpName.append(std::to_string(OffsetIdx));

                    ImGui::SetNextItemWidth(TextHeight * 10);
                    ImGui::InputText(OffsetInpName.c_str(), IdxOffset->OffsetName, MAX_PATH, IMGUI_INPUTTEXT_STYLE, [](ImGuiInputTextCallbackData* data) -> int {
                        ImWchar c = data->EventChar;
                        return !((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c == '_') || (c == '[') || (c == ']'));
                    });
                    ImGui::SameLine();

                    std::string SignatureInpName = "Signature##";
                    SignatureInpName.append(std::to_string(OffsetIdx));

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
        if (g_SelectedProcess.PID)
        {
            CreateGlodThread((LPTHREAD_START_ROUTINE)DumpOffsets);
        }
        else
        {
            MessageBoxA(*g_pMainWnd, "Select a process first", "Global Offset Dumper", MB_OK);
        }
    }
    
    std::string FpsText = std::to_string((int)floor(ImGui::GetIO().Framerate));
    FpsText.append(" FPS");
    ImGui::Text(FpsText.c_str());
}