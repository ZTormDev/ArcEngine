#pragma once

#include "../Platform/Window.h"
#include "../Renderer/Renderer.h"

namespace arc
{
    class Application
    {
    public:
        Application();
        ~Application();

        int Run();

    private:
        Window m_Window;
        Renderer m_Renderer;
    };
}
