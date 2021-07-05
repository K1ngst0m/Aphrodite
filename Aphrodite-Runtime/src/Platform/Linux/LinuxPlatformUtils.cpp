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

#ifdef APH_PLATFORM_LINUX

namespace Aph {
    namespace Utils{

        static std::string ExecCommand(const std::string_view& cmd) {
            std::array<char, 128> buffer{};
            std::string result;
            std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.data(), "r"), pclose); if (!pipe) {
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
    }

    std::string FileDialogs::OpenFile(const char* filter) {
        auto fileName = Utils::OpenFileDialogGetPath();
        if(!fileName.empty())
            fileName.erase(std::remove(fileName.begin(), fileName.end(), '\n'), fileName.end());
        return fileName;
    }

    std::string FileDialogs::SaveFile(const char* filter) {
        auto fileName = Utils::OpenFileDialogGetPath();
        if(!fileName.empty())
            fileName.erase(std::remove(fileName.begin(), fileName.end(), '\n'), fileName.end());
        return fileName;
    }

}// namespace Aph

#else
#error platform is not support!
#endif