#include "VulkanContext.h"

#include "../../Core/Log.h"
#include <SDL_vulkan.h>

namespace arc
{
    bool VulkanContext::Init(SDL_Window* window)
    {
        if (!CreateInstance(window))
            return false;

        if (!CreateSurface(window))
            return false;

        ARC_CORE_INFO("Vulkan context initialized");
        return true;
    }

    void VulkanContext::Shutdown()
    {
        if (m_Surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
            m_Surface = VK_NULL_HANDLE;
        }

        if (m_Instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(m_Instance, nullptr);
            m_Instance = VK_NULL_HANDLE;
        }
    }

    bool VulkanContext::CreateInstance(SDL_Window* window)
    {
        unsigned int extensionCount = 0;
        if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr))
        {
            ARC_CORE_ERROR("SDL_Vulkan_GetInstanceExtensions failed: {}", SDL_GetError());
            return false;
        }

        std::vector<const char*> extensions(extensionCount);
        if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions.data()))
        {
            ARC_CORE_ERROR("SDL_Vulkan_GetInstanceExtensions failed: {}", SDL_GetError());
            return false;
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Arc Engine";
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.pEngineName = "Arc Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);
        if (result != VK_SUCCESS)
        {
            ARC_CORE_ERROR("vkCreateInstance failed");
            return false;
        }

        return true;
    }

    bool VulkanContext::CreateSurface(SDL_Window* window)
    {
        if (!SDL_Vulkan_CreateSurface(window, m_Instance, &m_Surface))
        {
            ARC_CORE_ERROR("SDL_Vulkan_CreateSurface failed: {}", SDL_GetError());
            return false;
        }

        return true;
    }
}
