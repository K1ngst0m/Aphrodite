//
// Created by npchitman on 7/2/21.
//

#include "AssetBrowser.h"

#include <imgui.h>

#include "Aphrodite/Fonts/IconsFontAwesome5Pro.h"
#include "pch.h"

namespace Aph::Editor {

    std::filesystem::path AssetBrowser::m_AssetDirectoryPath("assets");
    std::filesystem::path AssetBrowser::m_CurrentRightPanelDirectoryPath(AssetBrowser::m_AssetDirectoryPath);

    void AssetBrowser::Init() {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontDefault();

        // merge in icons from Font Awesome
        static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS, 18.0f, &icons_config, icons_ranges);
    }

    void AssetBrowser::Draw() {
        ImGui::Begin(Style::Title::Project.data());
        ImGui::Columns(2, "Project", true);

        DrawLeftProjectPanel();

        ImGui::NextColumn();

        DrawRightProjectPanel();
    }

    bool AssetBrowser::IsDirectory(const std::filesystem::path& path) {
        return std::filesystem::is_directory(path);
    }

    bool AssetBrowser::IsDirectoryEmpty(const std::filesystem::path& path) {
        return IsDirectory(path) && std::filesystem::is_empty(path);
    }

    bool AssetBrowser::HasSubDirectory(const std::filesystem::path& path) {
        bool value = false;
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (IsDirectory(entry.path())) {
                value = true;
                break;
            }
        }

        return value;
    }

    void AssetBrowser::DrawLeftProjectPanel() {
        bool opened = ImGui::TreeNodeEx("leftProjectPanelAssets", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow, ICON_FA_FOLDER " assets");

        if (ImGui::IsItemClicked()) {
            m_CurrentRightPanelDirectoryPath = m_AssetDirectoryPath;
        }

        if (opened) {
            DrawRecursive(m_AssetDirectoryPath);
            ImGui::TreePop();
        }
    }

    void AssetBrowser::DrawRightProjectPanel() {
        DrawRightFilePathHeader(m_CurrentRightPanelDirectoryPath);

        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("RightProjectPanelVisor", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::Separator();

        std::string name, treeName;
        for (const auto& entry : std::filesystem::directory_iterator(m_CurrentRightPanelDirectoryPath)) {
            name = entry.path().filename().string();
            if (IsDirectory(entry.path())) {
                if (!IsDirectoryEmpty(entry.path())) {
                    treeName = ICON_FA_FOLDER + std::string(" ") + name;
                } else {
                    treeName = ICON_FA_FOLDER_MINUS + std::string(" ") + name;
                }

                if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Leaf, "%s", treeName.c_str())) {
                    if (ImGui::IsItemClicked()) {
                        m_CurrentRightPanelDirectoryPath = entry.path();
                    }
                    ImGui::TreePop();
                }
            } else {
                AssetFileType fileType = GetFileType(entry.path());
                switch (fileType) {
                    case AssetFileType::PNG: {
                        treeName = std::string(ICON_FA_PHOTO_VIDEO) + std::string(" ") + name;
                        if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Leaf, "%s", treeName.c_str())) {
                            ImGui::TreePop();
                        }
                        break;
                    }
                    case AssetFileType::SHADER: {
                        treeName = std::string(ICON_FA_CIRCLE) + std::string(" ") + name;
                        if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Leaf, "%s", treeName.c_str())) {
                            ImGui::TreePop();
                        }
                        break;
                    }
                    case AssetFileType::TTF: {
                        treeName = std::string(ICON_FA_FONT) + std::string(" ") + name;
                        if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Leaf, "%s", treeName.c_str())) {
                            ImGui::TreePop();
                        }
                        break;
                    }
                    case AssetFileType::SCENE: {
                        treeName = std::string(ICON_FA_ARCHIVE) + std::string(" ") + name;
                        if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Leaf, "%s", treeName.c_str())) {
                            ImGui::TreePop();
                        }
                        break;
                    }
                    case AssetFileType::NONE:
                        break;
                }
            }
        }

        ImGui::EndChild();
    }

    void AssetBrowser::DrawRecursive(const std::filesystem::path& path) {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (!IsDirectory(entry.path())) {
                continue;
            }

            std::string name = entry.path().filename().string();
            std::string treeName;

            if (!IsDirectoryEmpty(entry.path())) {
                treeName = ICON_FA_FOLDER + std::string(" ") + name;

                if (!HasSubDirectory(entry.path())) {
                    bool opened = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Leaf, "%s", treeName.c_str());

                    if (ImGui::IsItemClicked()) {
                        m_CurrentRightPanelDirectoryPath = entry.path();
                    }

                    DrawRecursive(entry.path());
                    ImGui::TreePop();
                } else {
                    bool opened = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_OpenOnArrow, "%s", treeName.c_str());

                    if (ImGui::IsItemClicked()) {
                        m_CurrentRightPanelDirectoryPath = entry.path();
                    }

                    if (opened) {
                        DrawRecursive(entry.path());
                        ImGui::TreePop();
                    }
                }
            } else {
                treeName = ICON_FA_FOLDER_MINUS + std::string(" ") + name;
                ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Leaf, "%s", treeName.c_str());
                if (ImGui::IsItemClicked()) {
                    m_CurrentRightPanelDirectoryPath = entry.path();
                }
                ImGui::TreePop();
            }
        }
    }

    void AssetBrowser::DrawRightFilePathHeader(const std::filesystem::path& path) {
        std::stringstream stream(path.string());
        std::string pathSegment;
        std::vector<std::string> pathSegmentList;

        char separator = (char) std::filesystem::path::preferred_separator;

        while (std::getline(stream, pathSegment, separator)) {
            pathSegmentList.push_back(pathSegment);
        }

        std::string cumulativePath;

        for (const auto& entry : pathSegmentList) {
            cumulativePath += entry + separator;

            if (ImGui::Button(entry.c_str())) {
                m_CurrentRightPanelDirectoryPath = cumulativePath;
            }

            ImGui::SameLine();
            ImGui::Text(">");
            ImGui::SameLine();
        }
        ImGui::NewLine();
    }

    AssetFileType AssetBrowser::GetFileType(const std::filesystem::path& path) {
        const auto& it = m_FileExtensionMap.find(path.filename().extension().string());
        if (it == m_FileExtensionMap.end()) {
            return AssetFileType::NONE;
        } else {
            return it->second;
        }
    }
}// namespace Aph
