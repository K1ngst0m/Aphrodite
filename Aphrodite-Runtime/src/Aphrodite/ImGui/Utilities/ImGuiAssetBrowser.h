//
// Created by npchitman on 7/2/21.
//

#ifndef APHRODITE_IMGUIASSETBROWSER_H
#define APHRODITE_IMGUIASSETBROWSER_H

namespace Aph {
    enum class AssetFileType {
        PNG = 0,
        GLSL = 1,
        TTF = 2,
        SCENE = 3,
        NONE = -1
    };

    class ImGuiAssetBrowser {
    public:
        static void Init();
        static void Draw();

    private:
        static bool IsDirectory(const std::filesystem::path& path);
        static bool IsDirectoryEmpty(const std::filesystem::path& path);
        static bool HasSubDirectory(const std::filesystem::path& path);


        static void DrawLeftProjectPanel();
        static void DrawRightProjectPanel();

        static void DrawRecursive(const std::filesystem::path& path);
        static void DrawRightFilePathHeader(const std::filesystem::path& path);

        static AssetFileType GetFileType(const std::filesystem::path& path);

    private:
        static std::filesystem::path m_AssetDirectoryPath;
        static std::filesystem::path m_CurrentRightPanelDirectoryPath;

        inline static std::unordered_map<std::string, AssetFileType> m_FileExtensionMap =
                {
                        {".png", AssetFileType::PNG},
                        {".glsl", AssetFileType::GLSL},
                        {".ttf", AssetFileType::TTF},
                        {".ghost", AssetFileType::SCENE}};
    };

}// namespace Aph


#endif//APHRODITE_IMGUIASSETBROWSER_H
