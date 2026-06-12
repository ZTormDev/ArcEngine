#include "Arc/Application.hpp"

#include "Arc/Input.hpp"
#include "Arc/Renderer.hpp"

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <SDL.h>
#include <SDL_syswm.h>

#include <algorithm>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <iostream>

namespace Arc
{
    namespace
    {
        std::string sdlError(const char* message)
        {
            return std::string(message) + ": " + SDL_GetError();
        }

        std::string defaultShaderDirectory()
        {
            char* basePath = SDL_GetBasePath();
            if(basePath == nullptr)
            {
                return "shaders";
            }

            std::string shaderDirectory = std::string(basePath) + "shaders";
            SDL_free(basePath);
            return shaderDirectory;
        }
    }

    Application::Application(ApplicationConfig config)
        : m_config(std::move(config))
    {
    }

    Application::~Application()
    {
        shutdown();
    }

    int Application::run()
    {
        try
        {
            initialize();
            onStart();

            const std::uint64_t performanceFrequency = SDL_GetPerformanceFrequency();
            const double targetFrameTime = 1.0 / 1000.0; // 1000 FPS limit

            while(m_running)
            {
                m_input->beginFrame();
                processEvents();

                std::uint64_t currentCounter = SDL_GetPerformanceCounter();
                double deltaSeconds = static_cast<double>(currentCounter - m_lastCounter) /
                    static_cast<double>(performanceFrequency);

                // Precise hybrid sleep/spin frame rate limiter
                if (deltaSeconds < targetFrameTime)
                {
                    double timeToWait = targetFrameTime - deltaSeconds;
                    std::uint32_t delayMs = static_cast<std::uint32_t>(timeToWait * 1000.0);
                    if (delayMs > 0)
                    {
                        SDL_Delay(delayMs);
                    }

                    do
                    {
                        currentCounter = SDL_GetPerformanceCounter();
                        deltaSeconds = static_cast<double>(currentCounter - m_lastCounter) /
                            static_cast<double>(performanceFrequency);
                    } while (deltaSeconds < targetFrameTime);
                }

                m_lastCounter = currentCounter;

                onUpdate(static_cast<float>(deltaSeconds));
                m_renderer->beginFrame();
                m_renderer->setFrameStats(static_cast<float>(deltaSeconds));
                onRender(*m_renderer);
                m_renderer->endFrame();
            }

            onShutdown();
            shutdown();
            return 0;
        }
        catch(const std::exception& error)
        {
            std::cerr << "Application Exception: " << error.what() << std::endl;
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Arc Engine", error.what(), m_window);
            shutdown();
            return -1;
        }
    }

    void Application::onStart()
    {
    }

    void Application::onUpdate(float deltaSeconds)
    {
        (void)deltaSeconds;
    }

    void Application::onRender(Renderer& renderer)
    {
        (void)renderer;
    }

    void Application::onResize(std::uint32_t width, std::uint32_t height)
    {
        (void)width;
        (void)height;
    }

    void Application::onShutdown()
    {
    }

    void Application::initialize()
    {
        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0)
        {
            throw std::runtime_error(sdlError("Failed to initialize SDL2"));
        }

        m_sdlInitialized = true;

        const std::uint32_t windowFlags = SDL_WINDOW_SHOWN |
            SDL_WINDOW_ALLOW_HIGHDPI |
            (m_config.resizable ? SDL_WINDOW_RESIZABLE : 0u);

        m_window = SDL_CreateWindow(
            m_config.title.c_str(),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            static_cast<int>(m_config.width),
            static_cast<int>(m_config.height),
            windowFlags);

        if(m_window == nullptr)
        {
            throw std::runtime_error(sdlError("Failed to create SDL2 window"));
        }

        SDL_SysWMinfo windowInfo;
        SDL_VERSION(&windowInfo.version);

        if(SDL_GetWindowWMInfo(m_window, &windowInfo) == SDL_FALSE)
        {
            throw std::runtime_error(sdlError("Failed to get native window handle"));
        }

        bgfx::PlatformData platformData{};

#if defined(_WIN32)
        platformData.nwh = windowInfo.info.win.window;
#else
        throw std::runtime_error("This Arc template maps SDL2 native windows for Windows/Visual Studio builds.");
#endif

        bgfx::Init init;
        init.type = bgfx::RendererType::Vulkan;
        init.vendorId = BGFX_PCI_ID_NONE;
        init.platformData = platformData;
        init.resolution.width = m_config.width;
        init.resolution.height = m_config.height;
        init.resolution.reset = BGFX_RESET_NONE;

        if(!bgfx::init(init))
        {
            throw std::runtime_error("Failed to initialize BGFX with Vulkan.");
        }

        m_bgfxInitialized = true;
        m_input = new Input();
        m_renderer = new Renderer();
        m_renderer->initialize({
            m_config.width,
            m_config.height,
            m_config.shaderDirectory.empty() ? defaultShaderDirectory() : m_config.shaderDirectory
        });

        m_running = true;
        m_lastCounter = SDL_GetPerformanceCounter();
    }

    void Application::processEvents()
    {
        SDL_Event event;

        while(SDL_PollEvent(&event) != 0)
        {
            if(event.type == SDL_QUIT)
            {
                m_running = false;
            }

            if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
            {
                m_running = false;
            }

            if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
            {
                mapKey(event.key.keysym.scancode, event.type == SDL_KEYDOWN);
            }

            if(event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
            {
                const bool pressed = event.type == SDL_MOUSEBUTTONDOWN;

                if(event.button.button == SDL_BUTTON_LEFT)
                {
                    m_input->setMouseButton(MouseButton::Left, pressed);
                }
                else if(event.button.button == SDL_BUTTON_RIGHT)
                {
                    m_input->setMouseButton(MouseButton::Right, pressed);
                }
                else if(event.button.button == SDL_BUTTON_MIDDLE)
                {
                    m_input->setMouseButton(MouseButton::Middle, pressed);
                }
            }

            if(event.type == SDL_MOUSEMOTION)
            {
                m_input->addMouseMotion(event.motion.xrel, event.motion.yrel);
            }

            if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                const auto width = static_cast<std::uint32_t>(std::max(event.window.data1, 1));
                const auto height = static_cast<std::uint32_t>(std::max(event.window.data2, 1));

                bgfx::reset(width, height, BGFX_RESET_NONE);
                m_renderer->resize(width, height);
                onResize(width, height);
            }
        }
    }

    void Application::shutdown()
    {
        if(m_renderer != nullptr)
        {
            m_renderer->shutdown();
            delete m_renderer;
            m_renderer = nullptr;
        }

        if(m_input != nullptr)
        {
            delete m_input;
            m_input = nullptr;
        }

        if(m_bgfxInitialized)
        {
            bgfx::shutdown();
            m_bgfxInitialized = false;
        }

        if(m_window != nullptr)
        {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }

        if(m_sdlInitialized)
        {
            SDL_Quit();
            m_sdlInitialized = false;
        }

        m_running = false;
    }

    Input& Application::input()
    {
        return *m_input;
    }

    const Input& Application::input() const
    {
        return *m_input;
    }

    Renderer& Application::renderer()
    {
        return *m_renderer;
    }

    const Renderer& Application::renderer() const
    {
        return *m_renderer;
    }

    std::uint32_t Application::width() const
    {
        return m_renderer != nullptr ? m_renderer->width() : m_config.width;
    }

    std::uint32_t Application::height() const
    {
        return m_renderer != nullptr ? m_renderer->height() : m_config.height;
    }

    void Application::quit()
    {
        m_running = false;
    }

    void Application::setMouseCaptured(bool captured)
    {
        if(m_mouseCaptured == captured)
        {
            return;
        }

        SDL_SetRelativeMouseMode(captured ? SDL_TRUE : SDL_FALSE);
        m_mouseCaptured = captured;
    }

    void Application::mapKey(int scancode, bool pressed)
    {
        switch(scancode)
        {
        case SDL_SCANCODE_W:
            m_input->setKey(Key::W, pressed);
            break;
        case SDL_SCANCODE_A:
            m_input->setKey(Key::A, pressed);
            break;
        case SDL_SCANCODE_S:
            m_input->setKey(Key::S, pressed);
            break;
        case SDL_SCANCODE_D:
            m_input->setKey(Key::D, pressed);
            break;
        case SDL_SCANCODE_Q:
            m_input->setKey(Key::Q, pressed);
            break;
        case SDL_SCANCODE_E:
            m_input->setKey(Key::E, pressed);
            break;
        case SDL_SCANCODE_LSHIFT:
        case SDL_SCANCODE_RSHIFT:
            m_input->setKey(Key::LeftShift, pressed);
            break;
        case SDL_SCANCODE_1:
            m_input->setKey(Key::Num1, pressed);
            break;
        case SDL_SCANCODE_2:
            m_input->setKey(Key::Num2, pressed);
            break;
        case SDL_SCANCODE_3:
            m_input->setKey(Key::Num3, pressed);
            break;
        case SDL_SCANCODE_I:
            m_input->setKey(Key::I, pressed);
            break;
        case SDL_SCANCODE_O:
            m_input->setKey(Key::O, pressed);
            break;
        case SDL_SCANCODE_K:
            m_input->setKey(Key::K, pressed);
            break;
        case SDL_SCANCODE_L:
            m_input->setKey(Key::L, pressed);
            break;
        case SDL_SCANCODE_U:
            m_input->setKey(Key::U, pressed);
            break;
        case SDL_SCANCODE_J:
            m_input->setKey(Key::J, pressed);
            break;
        case SDL_SCANCODE_ESCAPE:
            m_input->setKey(Key::Escape, pressed);
            break;
        default:
            break;
        }
    }
}
