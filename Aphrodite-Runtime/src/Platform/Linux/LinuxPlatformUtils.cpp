//
// Created by npchitman on 6/29/21.
//

#include "Aphrodite/Utils/PlatformUtils.h"
#include "pch.h"
#include "Aphrodite/Core/Application.h"

#ifdef APH_PLATFORM_LINUX

namespace Aph {
    namespace Utils{

        static std::string ExecCommand(const std::string_view& cmd) {
            std::array<char, 128> buffer{};
            std::string result;
            std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.data(), "r"), pclose);
            while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                result += buffer.data();
            }
            return result;
        }

        static std::string OpenFileDialogGetPath(const char * filter = "*") {
            return ExecCommand(fmt::format(
                    "zenity --file-selection --file-filter={}", filter));
        }
    }

    std::string FileDialogs::OpenFile(const char* filter) {
        auto fileName = Utils::OpenFileDialogGetPath(filter);
        if(!fileName.empty()) {
            fileName.erase(std::remove(fileName.begin(), fileName.end(), '\n'), fileName.end());
        }
        return fileName;
    }

    std::string FileDialogs::SaveFile(const char* filter) {
        auto fileName = Utils::OpenFileDialogGetPath(filter);
        if(!fileName.empty()) {
            fileName.erase(std::remove(fileName.begin(), fileName.end(), '\n'), fileName.end());
        }
        return fileName;
    }

}// namespace Aph
#else
#error platform is not support!
#endif
