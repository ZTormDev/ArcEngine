#pragma once

#include <SDL.h>
#include <vulkan/vulkan.h>
#include <vector>

namespace arc
{
    class VulkanContext
    {
    public:
        bool Init(SDL_Window* window);
        void Shutdown();

        VkInstance GetInstance() const { return m_Instance; }
        VkSurfaceKHR GetSurface() const { return m_Surface; }

    private:
        bool CreateInstance(SDL_Window* window);
        bool CreateSurface(SDL_Window* window);

    private:
        VkInstance m_Instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    };
}
