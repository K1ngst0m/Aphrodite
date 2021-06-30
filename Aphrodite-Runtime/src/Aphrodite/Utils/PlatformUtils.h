//
// Created by npchitman on 6/29/21.
//

#ifndef Aphrodite_ENGINE_PLATFORMUTILS_H
#define Aphrodite_ENGINE_PLATFORMUTILS_H

#include <string>
#include <optional>

namespace Aph {

    class FileDialogs {
    public:
        // These return empty strings if cancelled
        static std::optional<std::string> OpenFile(const char* filter);
        static std::optional<std::string> SaveFile(const char* filter);
    };

}// namespace Aph

#endif//Aphrodite_ENGINE_PLATFORMUTILS_H
