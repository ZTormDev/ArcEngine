#pragma once

#include <cstdint>
#include <string>

struct SDL_Window;

namespace Arc
{
    struct ApplicationConfig
    {
        std::string title = "Arc Engine";
        std::uint32_t width = 1280;
        std::uint32_t height = 720;
        bool resizable = true;
    };

    class Application
    {
    public:
        explicit Application(ApplicationConfig config = {});
        virtual ~Application();

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(Application&&) = delete;

        int run();

    protected:
        virtual void onStart();
        virtual void onUpdate(float deltaSeconds);
        virtual void onShutdown();

    private:
        void initialize();
        void processEvents();
        void render();
        void shutdown();

        ApplicationConfig m_config;
        SDL_Window* m_window = nullptr;
        bool m_running = false;
        bool m_sdlInitialized = false;
        bool m_bgfxInitialized = false;
        std::uint64_t m_lastCounter = 0;
    };
}
