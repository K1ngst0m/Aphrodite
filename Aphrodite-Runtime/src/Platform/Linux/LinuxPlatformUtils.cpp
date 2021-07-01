//
// Created by npchitman on 6/29/21.
//

#include <sstream>

#include "Aphrodite/Utils/PlatformUtils.h"
#include "GLFW/glfw3.h"
#include "pch.h"
//#define GLFW_EXPOSE_NATIVE_X11
//#include <GLFW/glfw3native.h>

#include "Aphrodite/Core/Application.h"

namespace Aph {

    static std::string ExecCommand(const std::string_view& cmd) {
        std::array<char, 128> buffer{};
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.data(), "r"), pclose);
        if (!pipe) {
            //            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

    static std::string OpenFileDialogGetPath() {
        return std::move(ExecCommand("zenity --file-selection"));
    }

    std::string FileDialogs::OpenFile(const char* filter) {
#if false
        OPENFILENAMEA ofn;
        CHAR szFile[260] = { 0 };
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow());
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
        if (GetOpenFileNameA(&ofn) == TRUE)
        {
            return ofn.lpstrFile;
        }
        return std::string();
#endif
        auto fileName = OpenFileDialogGetPath();

        return fileName;
    }

    std::string FileDialogs::SaveFile(const char* filter) {
#if false
        OPENFILENAMEA ofn;
        CHAR szFile[260] = { 0 };
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow());
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
        if (GetSaveFileNameA(&ofn) == TRUE)
        {
            return ofn.lpstrFile;
        }
        return std::string();
#endif
        auto fileName = OpenFileDialogGetPath();

        return fileName;
    }

}// namespace Aph