#include "Renderer.h"

#include "../Core/Log.h"
#include "../Platform/Window.h"

#include <filament/LightManager.h>
#include <filament/RenderableManager.h>
#include <filament/TransformManager.h>
#include <filament/Viewport.h>
#include <math/mat4.h>
#include <math/vec3.h>
#include <math/vec4.h>
#include <utils/EntityManager.h>

#include <SDL_syswm.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace arc
{
    namespace
    {
        struct CubeVertex
        {
            filament::math::float3 Position;
            filament::math::float4 Tangent;
            uint32_t Color;
        };

        std::vector<uint8_t> ReadBinaryFile(const std::string& path)
        {
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file)
                return {};

            const std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            std::vector<uint8_t> buffer(static_cast<size_t>(size));
            if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
                return {};

            return buffer;
        }

        // Tangent frame quaternion placeholders. Good enough for a prototype cube.
        // For production meshes, generate tangents during asset import.
        constexpr filament::math::float4 DefaultTangent = {1.0f, 0.0f, 0.0f, 1.0f};
        constexpr uint32_t WhiteColor = 0xffffffffu;
    }

    bool Renderer::Init(Window& window)
    {
        ARC_CORE_INFO("Initializing Filament with Vulkan backend");

        m_Engine = filament::Engine::create(filament::Engine::Backend::VULKAN);
        if (!m_Engine)
        {
            ARC_CORE_ERROR("Filament Engine::create(VULKAN) failed");
            return false;
        }

        void* nativeWindow = GetNativeWindowHandle(window);
        if (!nativeWindow)
        {
            ARC_CORE_ERROR("Could not get native window handle for Filament swapchain");
            return false;
        }

        m_SwapChain = m_Engine->createSwapChain(nativeWindow);
        m_Renderer = m_Engine->createRenderer();
        m_View = m_Engine->createView();
        m_Scene = m_Engine->createScene();

        if (!m_SwapChain || !m_Renderer || !m_View || !m_Scene)
        {
            ARC_CORE_ERROR("Failed to create Filament render objects");
            return false;
        }

        m_View->setScene(m_Scene);
        m_View->setViewport({0, 0, static_cast<uint32_t>(window.GetWidth()), static_cast<uint32_t>(window.GetHeight())});
        m_View->setPostProcessingEnabled(true);

        filament::AmbientOcclusionOptions aoOptions;
        aoOptions.enabled = true;
        m_View->setAmbientOcclusionOptions(aoOptions);

        filament::BloomOptions bloomOptions;
        bloomOptions.enabled = true;
        m_View->setBloomOptions(bloomOptions);

        CreateCamera(window);
        CreateSkybox();
        CreateSun();

        if (!CreateCube())
            return false;

        m_Initialized = true;
        ARC_CORE_INFO("Filament scene initialized: freecam, skybox, sun, cube");
        return true;
    }

    void Renderer::OnEvent(const SDL_Event& event, Window& window)
    {
        m_FreeCamera.OnEvent(event, window);
    }

    void Renderer::BeginFrame(Window& window, float deltaSeconds)
    {
        if (!m_Initialized)
            return;

        m_FreeCamera.Update(deltaSeconds);
        UpdateCamera(window);

        if (window.WasResized())
        {
            m_View->setViewport({0, 0, static_cast<uint32_t>(window.GetWidth()), static_cast<uint32_t>(window.GetHeight())});
            window.ClearResizeFlag();
        }

        if (m_Renderer->beginFrame(m_SwapChain))
        {
            m_Renderer->render(m_View);
        }
    }

    void Renderer::EndFrame()
    {
        if (m_Initialized && m_Renderer)
            m_Renderer->endFrame();
    }

    void Renderer::Shutdown()
    {
        if (!m_Engine)
            return;

        DestroySceneObjects();

        if (m_View)
        {
            m_Engine->destroy(m_View);
            m_View = nullptr;
        }

        if (m_Renderer)
        {
            m_Engine->destroy(m_Renderer);
            m_Renderer = nullptr;
        }

        if (m_SwapChain)
        {
            m_Engine->destroy(m_SwapChain);
            m_SwapChain = nullptr;
        }

        filament::Engine::destroy(&m_Engine);
        m_Engine = nullptr;
        m_Initialized = false;

        ARC_CORE_INFO("Renderer shutdown");
    }

    void Renderer::CreateCamera(Window& window)
    {
        auto& entityManager = utils::EntityManager::get();
        m_CameraEntity = entityManager.create();
        m_Camera = m_Engine->createCamera(m_CameraEntity);
        m_View->setCamera(m_Camera);
        UpdateCamera(window);
    }

    void Renderer::CreateSkybox()
    {
        m_Skybox = filament::Skybox::Builder()
            .color({0.035f, 0.055f, 0.085f, 1.0f})
            .build(*m_Engine);

        m_Scene->setSkybox(m_Skybox);
    }

    void Renderer::CreateSun()
    {
        auto& entityManager = utils::EntityManager::get();
        m_SunEntity = entityManager.create();

        filament::LightManager::Builder(filament::LightManager::Type::SUN)
            .color({1.0f, 0.956f, 0.84f})
            .intensity(110000.0f)
            .direction({-0.45f, -1.0f, -0.25f})
            .sunAngularRadius(0.53f)
            .sunHaloSize(10.0f)
            .castShadows(true)
            .build(*m_Engine, m_SunEntity);

        m_Scene->addEntity(m_SunEntity);
    }

    bool Renderer::CreateCube()
    {
        const std::string materialPath = "Assets/Engine/Materials/ArcDefault.filamat";
        const std::vector<uint8_t> materialData = ReadBinaryFile(materialPath);
        if (materialData.empty())
        {
            ARC_CORE_ERROR("Could not load material package: {}", materialPath);
            ARC_CORE_ERROR("CMake should compile Assets/Engine/Materials/ArcDefault.mat with matc before running.");
            return false;
        }

        m_CubeMaterial = filament::Material::Builder()
            .package(materialData.data(), materialData.size())
            .build(*m_Engine);

        m_CubeMaterialInstance = m_CubeMaterial->createInstance();
        m_CubeMaterialInstance->setParameter("baseColor", filament::math::float4{0.78f, 0.78f, 0.78f, 1.0f});
        m_CubeMaterialInstance->setParameter("roughness", 0.55f);
        m_CubeMaterialInstance->setParameter("metallic", 0.0f);

        constexpr float s = 1.0f;

        const std::array<CubeVertex, 24> vertices = {{
            {{-s,-s, s}, DefaultTangent, WhiteColor}, {{ s,-s, s}, DefaultTangent, WhiteColor}, {{ s, s, s}, DefaultTangent, WhiteColor}, {{-s, s, s}, DefaultTangent, WhiteColor},
            {{ s,-s,-s}, DefaultTangent, WhiteColor}, {{-s,-s,-s}, DefaultTangent, WhiteColor}, {{-s, s,-s}, DefaultTangent, WhiteColor}, {{ s, s,-s}, DefaultTangent, WhiteColor},
            {{-s,-s,-s}, DefaultTangent, WhiteColor}, {{-s,-s, s}, DefaultTangent, WhiteColor}, {{-s, s, s}, DefaultTangent, WhiteColor}, {{-s, s,-s}, DefaultTangent, WhiteColor},
            {{ s,-s, s}, DefaultTangent, WhiteColor}, {{ s,-s,-s}, DefaultTangent, WhiteColor}, {{ s, s,-s}, DefaultTangent, WhiteColor}, {{ s, s, s}, DefaultTangent, WhiteColor},
            {{-s, s, s}, DefaultTangent, WhiteColor}, {{ s, s, s}, DefaultTangent, WhiteColor}, {{ s, s,-s}, DefaultTangent, WhiteColor}, {{-s, s,-s}, DefaultTangent, WhiteColor},
            {{-s,-s,-s}, DefaultTangent, WhiteColor}, {{ s,-s,-s}, DefaultTangent, WhiteColor}, {{ s,-s, s}, DefaultTangent, WhiteColor}, {{-s,-s, s}, DefaultTangent, WhiteColor},
        }};

        const std::array<uint16_t, 36> indices = {{
             0,  1,  2,  0,  2,  3,
             4,  5,  6,  4,  6,  7,
             8,  9, 10,  8, 10, 11,
            12, 13, 14, 12, 14, 15,
            16, 17, 18, 16, 18, 19,
            20, 21, 22, 20, 22, 23,
        }};

        m_CubeVertexBuffer = filament::VertexBuffer::Builder()
            .vertexCount(vertices.size())
            .bufferCount(1)
            .attribute(filament::VertexAttribute::POSITION, 0, filament::VertexBuffer::AttributeType::FLOAT3, offsetof(CubeVertex, Position), sizeof(CubeVertex))
            .attribute(filament::VertexAttribute::TANGENTS, 0, filament::VertexBuffer::AttributeType::FLOAT4, offsetof(CubeVertex, Tangent), sizeof(CubeVertex))
            .attribute(filament::VertexAttribute::COLOR, 0, filament::VertexBuffer::AttributeType::UBYTE4, offsetof(CubeVertex, Color), sizeof(CubeVertex))
            .normalized(filament::VertexAttribute::COLOR)
            .build(*m_Engine);

        m_CubeVertexBuffer->setBufferAt(
            *m_Engine,
            0,
            filament::VertexBuffer::BufferDescriptor(vertices.data(), vertices.size() * sizeof(CubeVertex), nullptr));

        m_CubeIndexBuffer = filament::IndexBuffer::Builder()
            .indexCount(indices.size())
            .bufferType(filament::IndexBuffer::IndexType::USHORT)
            .build(*m_Engine);

        m_CubeIndexBuffer->setBuffer(
            *m_Engine,
            filament::IndexBuffer::BufferDescriptor(indices.data(), indices.size() * sizeof(uint16_t), nullptr));

        auto& entityManager = utils::EntityManager::get();
        m_CubeEntity = entityManager.create();

        filament::RenderableManager::Builder(1)
            .boundingBox({{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}})
            .material(0, m_CubeMaterialInstance)
            .geometry(0, filament::RenderableManager::PrimitiveType::TRIANGLES, m_CubeVertexBuffer, m_CubeIndexBuffer)
            .castShadows(true)
            .receiveShadows(true)
            .build(*m_Engine, m_CubeEntity);

        m_Scene->addEntity(m_CubeEntity);
        return true;
    }

    void Renderer::UpdateCamera(Window& window)
    {
        if (!m_Camera)
            return;

        const float aspect = static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight() > 0 ? window.GetHeight() : 1);
        m_Camera->setProjection(
            m_FreeCamera.GetFovDegrees(),
            aspect,
            m_FreeCamera.GetNearPlane(),
            m_FreeCamera.GetFarPlane(),
            filament::Camera::Fov::VERTICAL);

        const glm::vec3 eye = m_FreeCamera.GetPosition();
        const glm::vec3 center = eye + m_FreeCamera.GetForward();
        const glm::vec3 up = m_FreeCamera.GetUp();

        m_Camera->lookAt(
            filament::math::double3{eye.x, eye.y, eye.z},
            filament::math::double3{center.x, center.y, center.z},
            filament::math::double3{up.x, up.y, up.z});
    }

    void Renderer::DestroySceneObjects()
    {
        if (m_Scene)
        {
            if (m_CubeEntity) m_Scene->remove(m_CubeEntity);
            if (m_SunEntity) m_Scene->remove(m_SunEntity);
        }

        if (m_CubeEntity)
        {
            m_Engine->destroy(m_CubeEntity);
            utils::EntityManager::get().destroy(m_CubeEntity);
            m_CubeEntity.clear();
        }

        if (m_SunEntity)
        {
            m_Engine->destroy(m_SunEntity);
            utils::EntityManager::get().destroy(m_SunEntity);
            m_SunEntity.clear();
        }

        if (m_CubeVertexBuffer)
        {
            m_Engine->destroy(m_CubeVertexBuffer);
            m_CubeVertexBuffer = nullptr;
        }

        if (m_CubeIndexBuffer)
        {
            m_Engine->destroy(m_CubeIndexBuffer);
            m_CubeIndexBuffer = nullptr;
        }

        if (m_CubeMaterialInstance)
        {
            m_Engine->destroy(m_CubeMaterialInstance);
            m_CubeMaterialInstance = nullptr;
        }

        if (m_CubeMaterial)
        {
            m_Engine->destroy(m_CubeMaterial);
            m_CubeMaterial = nullptr;
        }

        if (m_Skybox)
        {
            m_Engine->destroy(m_Skybox);
            m_Skybox = nullptr;
        }

        if (m_Camera)
        {
            m_Engine->destroyCameraComponent(m_CameraEntity);
            m_Camera = nullptr;
        }

        if (m_CameraEntity)
        {
            utils::EntityManager::get().destroy(m_CameraEntity);
            m_CameraEntity.clear();
        }

        if (m_Scene)
        {
            m_Engine->destroy(m_Scene);
            m_Scene = nullptr;
        }
    }

    void* Renderer::GetNativeWindowHandle(Window& window)
    {
        SDL_SysWMinfo info;
        SDL_VERSION(&info.version);

        if (!SDL_GetWindowWMInfo(window.GetNativeHandle(), &info))
            return nullptr;

    #if defined(_WIN32)
        return static_cast<void*>(info.info.win.window);
    #elif defined(__APPLE__)
        return nullptr;
    #elif defined(__linux__)
        // This prototype keeps the native-window extraction explicit.
        // For Linux, adapt this for X11/Wayland using Filament's platform helper or your own backend wrapper.
        return nullptr;
    #else
        return nullptr;
    #endif
    }
}
