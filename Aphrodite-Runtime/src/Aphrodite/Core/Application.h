// Application.h

// application class:
// - control how game create and delete
// - handle events
// - manage layer
// - control game loop
// - lifecycle function
// - Window event callback


#ifndef Aphrodite_APPLICATION_H
#define Aphrodite_APPLICATION_H

#include "Aphrodite/Core/Base.h"
#include "Aphrodite/Core/LayerStack.h"
#include "Aphrodite/Core/TimeStep.h"
#include "Aphrodite/Core/Window.h"
#include "Aphrodite/Events/ApplicationEvent.h"
#include "Aphrodite/Events/Event.h"
#include "Aphrodite/Events/KeyEvent.h"
#include "Aphrodite/Events/MouseEvent.h"
#include "Aphrodite/ImGui/ImGuiLayer.h"

int main(int argc, char **argv);

namespace Aph {
    struct ApplicationCommandLineArgs {
        int Count = 0;
        char **Args = nullptr;

        const char *operator[](int index) const {
            APH_CORE_ASSERT(index < Count);
            return Args[index];
        }
    };

    class Application {
    public:
        explicit Application(const std::string &name = "Aph-Runtime App");
        explicit Application(const std::string &name = "Aph-Runtime App", ApplicationCommandLineArgs args = ApplicationCommandLineArgs());

        virtual ~Application();

        void OnEvent(Event &e);

        void PushLayer(Layer *layer);

        void PushOverlay(Layer *layer);

        Window &GetWindow() { return *m_Window; }

        void Close();

        ImGuiLayer *GetImGuiLayer() { return m_ImGuiLayer; }
        static Application &Get() { return *s_Instance; }

        ApplicationCommandLineArgs GetCommandLineArgs() const { return m_CommandLineArgs; }

    private:
        void Run();
        bool OnWindowClose(WindowCloseEvent &e);
        bool OnWindowResize(WindowResizeEvent &e);

    private:
        ApplicationCommandLineArgs m_CommandLineArgs;
        Scope<Window> m_Window;
        ImGuiLayer *m_ImGuiLayer{};
        bool m_Running = true;
        bool m_Minimized = false;
        LayerStack m_LayerStack;

        float m_LastFrameTime = 0.0f;

    private:
        static Application *s_Instance;
        friend int ::main(int argc, char **argv);
    };

    // to be defined in client
    Application *CreateApplication(ApplicationCommandLineArgs args);
}// namespace Aph-Runtime

#endif// Aphrodite_APPLICATION_H