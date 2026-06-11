#pragma once

#include "Camera/FreeCamera.h"

#include <SDL.h>

#include <filament/Box.h>
#include <filament/Camera.h>
#include <filament/Engine.h>
#include <filament/IndexBuffer.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/Skybox.h>
#include <filament/SwapChain.h>
#include <filament/VertexBuffer.h>
#include <filament/View.h>
#include <utils/Entity.h>

#include <memory>

namespace arc
{
    class Window;

    class Renderer
    {
    public:
        bool Init(Window& window);
        void OnEvent(const SDL_Event& event, Window& window);
        void BeginFrame(Window& window, float deltaSeconds);
        void EndFrame();
        void Shutdown();

    private:
        void CreateCamera(Window& window);
        void CreateSkybox();
        bool CreateCube();
        void CreateSun();
        void UpdateCamera(Window& window);
        void DestroySceneObjects();

        static void* GetNativeWindowHandle(Window& window);

    private:
        filament::Engine* m_Engine = nullptr;
        filament::Renderer* m_Renderer = nullptr;
        filament::SwapChain* m_SwapChain = nullptr;
        filament::View* m_View = nullptr;
        filament::Scene* m_Scene = nullptr;
        filament::Camera* m_Camera = nullptr;
        utils::Entity m_CameraEntity;

        filament::VertexBuffer* m_CubeVertexBuffer = nullptr;
        filament::IndexBuffer* m_CubeIndexBuffer = nullptr;
        filament::Material* m_CubeMaterial = nullptr;
        filament::MaterialInstance* m_CubeMaterialInstance = nullptr;
        utils::Entity m_CubeEntity;

        filament::Skybox* m_Skybox = nullptr;
        utils::Entity m_SunEntity;

        FreeCamera m_FreeCamera;
        bool m_Initialized = false;
    };
}
