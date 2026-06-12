#include "Arc/Application.hpp"

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <SDL.h>
#include <SDL_syswm.h>

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

namespace Arc
{
    namespace
    {
        std::string sdlError(const char* message)
        {
            return std::string(message) + ": " + SDL_GetError();
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

            while(m_running)
            {
                processEvents();

                const std::uint64_t currentCounter = SDL_GetPerformanceCounter();
                const float deltaSeconds = static_cast<float>(currentCounter - m_lastCounter) /
                    static_cast<float>(SDL_GetPerformanceFrequency());
                m_lastCounter = currentCounter;

                onUpdate(deltaSeconds);
                render();
            }

            onShutdown();
            shutdown();
            return 0;
        }
        catch(const std::exception& error)
        {
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
        init.resolution.reset = BGFX_RESET_VSYNC;

        if(!bgfx::init(init))
        {
            throw std::runtime_error("Failed to initialize BGFX with Vulkan.");
        }

        m_bgfxInitialized = true;

        bgfx::setDebug(BGFX_DEBUG_TEXT);
        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x101820ff, 1.0f, 0);
        bgfx::setViewRect(0, 0, 0, static_cast<std::uint16_t>(m_config.width), static_cast<std::uint16_t>(m_config.height));

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

            if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                const auto width = static_cast<std::uint32_t>(std::max(event.window.data1, 1));
                const auto height = static_cast<std::uint32_t>(std::max(event.window.data2, 1));

                bgfx::reset(width, height, BGFX_RESET_VSYNC);
                bgfx::setViewRect(0, 0, 0, static_cast<std::uint16_t>(width), static_cast<std::uint16_t>(height));
            }
        }
    }

    void Application::render()
    {
        bgfx::touch(0);
        bgfx::dbgTextClear();
        bgfx::dbgTextPrintf(1, 1, 0x4f, "Arc Engine");
        bgfx::dbgTextPrintf(1, 2, 0x0f, "Renderer: %s", bgfx::getRendererName(bgfx::getRendererType()));
        bgfx::dbgTextPrintf(1, 3, 0x0f, "Press Esc to quit.");
        bgfx::frame();
    }

    void Application::shutdown()
    {
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
}
