//
// Created by npchitman on 7/8/21.
//

#ifndef APHRODITE_UIDRAWER_H
#define APHRODITE_UIDRAWER_H

#include <functional>
#include <string>
#include <glm/gtc/type_ptr.hpp>

namespace Aph::Editor {
    class UIDrawer {
    public:
        static void PushID();
        static void PopID();

        static void BeginPropertyGrid();
        static void Property(const char* label);
        static bool Property(const char* label, std::string& value);
        static void Property(const char* label, const char* value);
        static bool Property(const char* label, int& value);
        static bool Property(const char* label, int& value, int min, int max);
        static bool Property(const char* label, float& value, float delta = 0.1f);
        static bool Property(const char* label, float& value, float min, float max, const char* fmt = "%.3f");
        static bool Property(const char* label, glm::vec2& value, float delta = 0.1f);
        static bool Property(const char* label, bool& flag);
        static void EndPropertyGrid();

        using APH_UIFUNC = std::function<void(void)>;

        static void Draw(const APH_UIFUNC & uiPushFunc, const APH_UIFUNC& uiPopFunc, const APH_UIFUNC& uiDrawFunc);
        static void DrawGrid(const APH_UIFUNC & func);
        static void DrawGrid(const std::string& name, const APH_UIFUNC& func);
    };
}// namespace Aph::Editor


#endif//APHRODITE_UIDRAWER_H
