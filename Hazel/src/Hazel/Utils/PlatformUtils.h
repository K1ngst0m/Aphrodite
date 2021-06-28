//
// Created by npchitman on 6/29/21.
//

#ifndef HAZEL_ENGINE_PLATFORMUTILS_H
#define HAZEL_ENGINE_PLATFORMUTILS_H

#include <string>
#include <optional>

namespace Hazel {

    class FileDialogs {
    public:
        // These return empty strings if cancelled
        static std::optional<std::string> OpenFile(const char* filter);
        static std::optional<std::string> SaveFile(const char* filter);
    };

}// namespace Hazel

#endif//HAZEL_ENGINE_PLATFORMUTILS_H
