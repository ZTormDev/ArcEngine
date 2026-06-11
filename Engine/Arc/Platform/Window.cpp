#include "Window.h"

#include "../Core/Log.h"

namespace arc
{
    Window::~Window()
    {
        Shutdown();
    }

    bool Window::Create(const WindowCreateInfo& info)
    {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0)
        {
            ARC_CORE_ERROR("SDL_Init failed: {}", SDL_GetError());
            return false;
        }

        m_Width = info.Width;
        m_Height = info.Height;

        m_Window = SDL_CreateWindow(
            info.Title.c_str(),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            info.Width,
            info.Height,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

        if (!m_Window)
        {
            ARC_CORE_ERROR("SDL_CreateWindow failed: {}", SDL_GetError());
            return false;
        }

        ARC_CORE_INFO("Window created: {} ({}x{})", info.Title, info.Width, info.Height);
        return true;
    }

    void Window::PollEvents(const EventCallback& callback)
    {
        m_Resized = false;

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (callback)
                callback(event);

            switch (event.type)
            {
                case SDL_QUIT:
                    m_Running = false;
                    break;

                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                        event.window.event == SDL_WINDOWEVENT_RESIZED)
                    {
                        m_Width = event.window.data1;
                        m_Height = event.window.data2;
                        m_Resized = true;
                    }
                    break;

                default:
                    break;
            }
        }
    }

    void Window::CaptureMouse(bool capture)
    {
        SDL_SetRelativeMouseMode(capture ? SDL_TRUE : SDL_FALSE);
    }

    void Window::Shutdown()
    {
        if (m_Window)
        {
            SDL_DestroyWindow(m_Window);
            m_Window = nullptr;
        }

        SDL_Quit();
    }
}
