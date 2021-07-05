//
// Created by npchitman on 6/24/21.
//

#include <Aphrodite/Core/EntryPoint.h>

#include <Aphrodite.hpp>

#include "EditorLayer.h"

namespace Aph {

    class AphroditeEditor : public Application {
    public:
        explicit AphroditeEditor(ApplicationCommandLineArgs args)
            : Application("AphroditeEditor", args) {
            PushLayer(new EditorLayer());
        }

        ~AphroditeEditor() override = default;
    };

    Application* CreateApplication(ApplicationCommandLineArgs args) {
        return new AphroditeEditor(args);
    }

}// namespace Aph
