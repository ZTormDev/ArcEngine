#include "Application.h"

#include "Log.h"

#include <SDL_timer.h>

namespace arc
{
    Application::Application()
    {
        Log::Init();
    }

    Application::~Application()
    {
        m_Renderer.Shutdown();
    }

    int Application::Run()
    {
        WindowCreateInfo windowInfo;
        windowInfo.Title = "Arc Engine - Filament Prototype";
        windowInfo.Width = 1280;
        windowInfo.Height = 720;

        if (!m_Window.Create(windowInfo))
            return 1;

        if (!m_Renderer.Init(m_Window))
            return 2;

        Uint64 previousCounter = SDL_GetPerformanceCounter();
        const double frequency = static_cast<double>(SDL_GetPerformanceFrequency());

        while (m_Window.IsRunning())
        {
            Uint64 currentCounter = SDL_GetPerformanceCounter();
            const float deltaSeconds = static_cast<float>((currentCounter - previousCounter) / frequency);
            previousCounter = currentCounter;

            m_Window.PollEvents([this](const SDL_Event& event)
            {
                m_Renderer.OnEvent(event, m_Window);
            });

            m_Renderer.BeginFrame(m_Window, deltaSeconds);
            m_Renderer.EndFrame();
        }

        return 0;
    }
}
