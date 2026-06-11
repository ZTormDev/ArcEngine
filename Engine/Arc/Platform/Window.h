#pragma once

#include <SDL.h>
#include <functional>
#include <string>

namespace arc
{
    struct WindowCreateInfo
    {
        std::string Title = "Arc Engine";
        int Width = 1280;
        int Height = 720;
    };

    class Window
    {
    public:
        using EventCallback = std::function<void(const SDL_Event&)>;

        Window() = default;
        ~Window();

        bool Create(const WindowCreateInfo& info);
        void PollEvents(const EventCallback& callback = nullptr);
        void Shutdown();

        void CaptureMouse(bool capture);

        bool IsRunning() const { return m_Running; }
        SDL_Window* GetNativeHandle() const { return m_Window; }
        int GetWidth() const { return m_Width; }
        int GetHeight() const { return m_Height; }
        bool WasResized() const { return m_Resized; }
        void ClearResizeFlag() { m_Resized = false; }

    private:
        SDL_Window* m_Window = nullptr;
        bool m_Running = true;
        bool m_Resized = false;
        int m_Width = 0;
        int m_Height = 0;
    };
}
