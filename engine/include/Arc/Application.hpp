#pragma once

#include <cstdint>
#include <string>

struct SDL_Window;

namespace Arc
{
    class Input;
    class Renderer;

    struct ApplicationConfig
    {
        std::string title = "Arc Engine";
        std::uint32_t width = 1280;
        std::uint32_t height = 720;
        bool resizable = true;
        std::string shaderDirectory;
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
        [[nodiscard]] Input& input();
        [[nodiscard]] const Input& input() const;
        [[nodiscard]] Renderer& renderer();
        [[nodiscard]] const Renderer& renderer() const;
        [[nodiscard]] std::uint32_t width() const;
        [[nodiscard]] std::uint32_t height() const;

        void quit();
        void setMouseCaptured(bool captured);

        virtual void onStart();
        virtual void onUpdate(float deltaSeconds);
        virtual void onRender(Renderer& renderer);
        virtual void onResize(std::uint32_t width, std::uint32_t height);
        virtual void onShutdown();

    private:
        void initialize();
        void processEvents();
        void shutdown();
        void mapKey(int scancode, bool pressed);

        ApplicationConfig m_config;
        SDL_Window* m_window = nullptr;
        Input* m_input = nullptr;
        Renderer* m_renderer = nullptr;
        bool m_running = false;
        bool m_sdlInitialized = false;
        bool m_bgfxInitialized = false;
        bool m_mouseCaptured = false;
        std::uint64_t m_lastCounter = 0;
    };
}
