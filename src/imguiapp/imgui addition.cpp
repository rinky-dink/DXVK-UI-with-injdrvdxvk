#include "pch.h"

void ComboListVer(type typebin, const bool& theard_status, std::string name, ImVec2 size, FlagsState flag)
{
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 textPosition(windowSize.x * 0.5f, windowSize.y * 0.5f);
    ImVec2 spinnerPosition(windowSize.x * 0.5f, windowSize.y * 0.5f + ImGui::GetTextLineHeightWithSpacing());

    

    std::string comboid{ "##" + name };
    if (ImGui::BeginListBox(comboid.c_str(), size)) { // 420, 5 + 5 * ImGui::GetTextLineHeightWithSpacing()
        if (!theard_status)
        {
            ImVec2 contentSize = ImGui::GetContentRegionAvail();
            ImVec2 spinnerSize(75, 36);
            ImVec2 textSize = ImGui::CalcTextSize("Загрузка...");
            ImVec2 textPosition((contentSize.x - textSize.x) * 0.5f, (contentSize.y - textSize.y-spinnerSize.y) * 0.5f);

            ImGui::SetCursorPos(textPosition);
            ImGui::Text("Загрузка...");

             // Здесь вы можете указать размер круглашка
            ImVec2 spinnerPosition((contentSize.x - spinnerSize.x) * 0.5f, (contentSize.y - textSize.y - spinnerSize.y) * 0.5f);

            ImGui::SetCursorPos(spinnerPosition);
            ImSpinner::SpinnerPatternEclipse("SpinnerPatternEclipse", 36, 2, { 1.f, 1.f, 1.f, 1.f }, 7, 2, 2.f, 0.f);


        }
        else
        {

            bool ShowOnlyInstalled = HasFlag(Flag_ShowOnlyInstalled, flag);
            bool ModifyWindow = HasFlag(Flag_ModifyWindow, flag);


            bool installedversions{ false };
            if (ShowOnlyInstalled)
                for (auto& i : dGame.data[typebin])
                {
                    if (i.flag == installed)
                    {
                        installedversions = true;
                        break;
                    }
                }
        
            if (!installedversions && ShowOnlyInstalled)
            {
                ImVec2 contentSize = ImGui::GetContentRegionAvail();
                ImVec2 buttonSize(200, 75);
                ImVec2 buttonPosition((contentSize.x - buttonSize.x) * 0.5f, (contentSize.y - buttonSize.y) * 0.5f);

                ImGui::SetCursorPos(buttonPosition);
                if (ImGui::Button("Открыть менджер версий", buttonSize))
                {
                    ver_manager = !ver_manager;
                }
            }
            else
            {
                auto processEntry = [&](auto& i, auto& key) {
                    if (((Flag_ShowOnlyInstalled & flag) != 0) && i.flag != installed)
                        return;

                    ImVec4 activeButtonColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
                    ImGui::PushStyleColor(ImGuiCol_Header, i.pending ? ImVec4(0.85f, 0.85f, 0.85f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f)); // Светло-серый цвет
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.8f, 0.8f, 0.8f, 1.0f)); // Еще более светлый серый цвет при наведении
                    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.9f, 0.9f, 0.9f, 1.0f)); // Очень светлый серый цвет при активации




                    ImVec4 textcolor;

                    if (i.flag == none)
                        textcolor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
                    else if (i.flag == installed)
                        textcolor = ImVec4(0.0f / 255.0f, 109.0f / 255.0f, 0.0f / 255.0f, 1.0f);
                    else if (i.flag == error)
                        textcolor = ImVec4(220.0f / 255.0f, 20.0f / 255.0f, 60.0f / 255.0f, 1.0f);  // Crimson


                    ImGui::PushStyleColor(ImGuiCol_Text, textcolor);



                    bool selected = ModifyWindow && i.pending && i.flag == installed;

                    std::string formated = ((selected) ? (key == DXVK ? "DXVK: " : key == DXVK_GPLASYNC ? "DXVK_ASYNC: " : "UNKNOWN") : "") + i.getversionFormated() /*+ ( ver[i].flag == installed? "(Установлено)" : "")*/;


                    if (ModifyWindow ? selected : true)
                    {

                        if (ModifyWindow && i.modify == modify_types::reinstall)
                        {
                            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.17f, 0.27f, 0.7f, 0.9f));
                            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.22f, 0.32f, 0.7f, 0.9f));
                            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.1f, 0.2f, 0.7f, 0.9f));
                        }
                        else if (ModifyWindow && i.modify == modify_types::del)
                        {
                            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.7f, 0.17f, 0.17f, 0.9f));
                            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.7f, 0.22f, 0.22f, 0.9f));
                            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.7f, 0.1f, 0.1f, 0.9f));
                        }

                        if (ImGui::Selectable(formated.c_str(), &i.pending, ((Flag_None & flag) != 0) ? ImGuiSelectableFlags_AllowDoubleClick : 0)) {

                            //if (!ModifyWindow && ImGui::IsMouseDoubleClicked(0))
                            //{
                            //    std::cout << "double click\n";
                            //    for (auto& x : ver)
                            //    {
                            //        x.pending = false;
                            //    }
                            //    ver[i].pending = !ver[i].pending;
                            //    folderPath = ver[i].getfoldername();
                            //    ChangeDllPath = true;

                            //}

                            if (ModifyWindow)
                            {
                                i.pending = true;
                                ++i.modify;
                            }
                            else if (ShowOnlyInstalled)
                            {
                                for (auto& j : dGame.data[typebin])
                                {
                                    j.pending = false;
                                }
                                i.pending = true;
                                dGame.dlls_FolderPath = i.getfoldername();
                                ChangeDllPath = true;
                            }

                        }
                        if (ModifyWindow && (i.modify == modify_types::reinstall || i.modify == modify_types::del))
                        {
                           // ImGui::PopStyleColor(3); // Подразумевается, что вы пушите три цветовых стиля
                        }


                        //if (deinmodify)
                        //    ImGui::PopStyleColor();

                    }
                    ImGui::PopStyleColor(10);
                };

                if (ModifyWindow) {
                    for (auto& [key, vec] : dGame.data) {
                        for (auto& i : vec) {
                            processEntry(i, key);
                        }
                    }
                }
                else {
                    for (auto& i : dGame.data[typebin]) {
                        processEntry(i, typebin);
                    }
                }
            }
        }
        ImGui::EndListBox();
    }
}
//
//void deinmodifyModal(std::vector<ReleaseInfo>& ver, const bool& theard_status){
//
//    if (buttonmask.i[1] == 0)
//    {
//        ImGui::CloseCurrentPopup();
//        return;
//    }
//
//    if (ImGui::BeginPopupModal("1233", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiPopupFlags_NoOpenOverExistingPopup))
//    {
//        if (ImGui::IsMouseClicked(0) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
//        {
//            ImGui::CloseCurrentPopup();
//        }
//
//        int countX = std::count_if(ver.begin(), ver.end(), [](const auto& elem) {
//            return elem.pending && elem.flag == installed;
//            });
//
//        // Подсчитываем количество элементов, удовлетворяющих условию ver[i].pending && elem.modify == modify_types::none && ver[i].flag == installed
//        int countY = std::count_if(ver.begin(), ver.end(), [](const auto& elem) {
//            return elem.pending && elem.modify == modify_types::none && elem.flag == installed;
//            });
//
//        ComboListVer(ver, theard_status, "deinmodify",  0, ver.size(), Flag_ModifyWindow);
//
//        /*for (auto s : emb)
//        {
//            if (s.pending && s.flag == installed)
//            {
//                std::string str = (s.typebin == DXVK ? "DXVK: " : s.typebin == VKD3D ? "VKD3D: " : "UNKNOWN") + s.version;
//                ImGui::Text(str.c_str());
//            }
//        }*/
//
//
//        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.17f, 0.17f, 1.0f));
//        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.22f, 0.22f, 1.0f));
//        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
//
//
//        if (ImGui::Button(countX == countY ? "Удалить всё" : "Удалить выбранное", ImVec2(200, 50)))
//        {
//            for (auto& dsa : ver)
//            {
//                if (dsa.modify == modify_types::del || (countX == countY && dsa.pending))
//                {
//                    if (dsa.flag == install_types::installed)
//                    {
//                        std::cout << dsa.getfoldername() << std::endl;
//
//                        try {
//                            fs::remove_all(dsa.getfoldername());
//                            dsa.clear();
//                            std::cout << "Файл успешно удален.\n";
//                        }
//                        catch (const fs::filesystem_error& e) {
//                            dsa.flag = install_types::error;
//                            std::cerr << "Не удалось удалить файл: " << e.what() << "\n";
//                        }
//                    }
//
//                }
//
//            }
//
//        }
//
//        ImGui::SameLine();
//
//        bool s = count_download != 0;
//
//        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.17f, 0.27f, 0.7f, s ? 0.4f : 1.0f));
//        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.32f, 0.7f, s ? 0.4f : 1.0f));
//        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.2f, 0.7f, s ? 0.4f : 1.0f));
//
//        if (ImGui::Button(countX == countY ? "Переустановить всё" : "Переустановить выбранное", ImVec2(200, 50)))
//        {
//            if (!s)
//            {
//                for (auto& dsa : ver)
//                {
//                    if (countX == countY && dsa.pending)
//                        dsa.modify = modify_types::reinstall;
//                }
//                std::thread downloadThread(DownloadInThreads, std::ref(ver), 5);
//                downloadThread.detach();
//            }
//        }
//        ImGui::PopStyleColor(6);
//
//        ImGui::EndPopup();
//    }
//}

void DarkTheme()
{
    // ==[ STYLE ]==
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(5, 5);
    style.FramePadding = ImVec2(5, 5);
    style.ItemSpacing = ImVec2(5, 5);
    style.ItemInnerSpacing = ImVec2(2, 2);
    style.TouchExtraPadding = ImVec2(0, 0);
    style.IndentSpacing = 0;
    style.ScrollbarSize = 10;
    style.GrabMinSize = 10;

    // ==[ BORDER ]==
    style.WindowBorderSize = 1;
    style.ChildBorderSize = 1;
    style.PopupBorderSize = 1;
    style.FrameBorderSize = 1;
    style.TabBorderSize = 1;

    // ==[ ROUNDING ]==
    style.WindowRounding = 5;
    style.ChildRounding = 5;
    style.FrameRounding = 5;
    style.PopupRounding = 5;
    style.ScrollbarRounding = 5;
    style.GrabRounding = 5;
    style.TabRounding = 5;

    // ==[ ALIGN ]==
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.5f, 0.5f);

    // ==[ COLORS ]==
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.26f, 0.54f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.26f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.26f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.21f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.21f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.21f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.70f);
}

void LightTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowPadding = ImVec2(8, 8);
    style.WindowRounding = 6.0f;
    style.ChildRounding = 5.0f;
    style.FramePadding = ImVec2(5, 3);
    style.FrameRounding = 3.0f;
    style.ItemSpacing = ImVec2(5, 4);
    style.ItemInnerSpacing = ImVec2(4, 4);
    style.IndentSpacing = 21.0f;
    style.ScrollbarSize = 10.0f;
    style.ScrollbarRounding = 13.0f;
    style.GrabMinSize = 8.0f;
    style.GrabRounding = 1.0f;
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.5f, 0.5f);

    colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f); // Фон рамки
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f); // Фон рамки при наведении
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f); // Фон рамки при активации
    colors[ImGuiCol_TitleBg] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.60f, 0.60f, 0.60f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.702f, 0.702f, 0.702f, 1.000f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.861f, 0.861f, 0.861f, 1.000f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.900f, 0.900f, 0.900f, 1.000f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f); // Цвет кнопки
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);

    colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}