//
// Created by npchitman on 7/2/21.
//

#ifndef APHRODITE_ASSETBROWSER_H
#define APHRODITE_ASSETBROWSER_H

#include <Aphrodite.hpp>

namespace Aph::Editor {
    enum class AssetFileType {
        PNG = 0,
        SHADER = 1,
        TTF = 2,
        SCENE = 3,
        NONE = -1
    };

    class AssetBrowser {
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
                        {".glsl", AssetFileType::SHADER},
                        {".vert", AssetFileType::SHADER},
                        {".frag", AssetFileType::SHADER},
                        {".ttf", AssetFileType::TTF},
                        {".sce", AssetFileType::SCENE}};
    };

}// namespace Aph


#endif//APHRODITE_ASSETBROWSER_H
