#pragma once

#include "Arc/Camera.hpp"
#include "Arc/Math.hpp"

#include <cstdint>
#include <string>

namespace Arc
{
    struct RendererConfig
    {
        std::uint32_t width = 1280;
        std::uint32_t height = 720;
        std::string shaderDirectory;
    };

    struct DirectionalLight
    {
        Vec3 direction = normalize(Vec3{ -0.35f, -1.0f, -0.25f });
        Color color{ 1.0f, 0.92f, 0.72f, 1.0f };
        float intensity = 5.0f;
    };

    struct Material
    {
        Color baseColor{ 1.0f, 1.0f, 1.0f, 1.0f };
        float metallic = 0.0f;
        float roughness = 0.55f;
        float emissive = 0.0f;
    };

    struct Skybox
    {
        Color horizon{ 0.48f, 0.68f, 0.95f, 1.0f };
        Color zenith{ 0.04f, 0.12f, 0.28f, 1.0f };
        Color sunGlow{ 1.0f, 0.78f, 0.38f, 1.0f };
    };

    struct RenderStats
    {
        std::uint32_t drawCalls = 0;
        std::uint32_t meshDraws = 0;
        std::uint32_t shadowDraws = 0;
    };

    class Renderer
    {
    public:
        Renderer() = default;
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        void initialize(const RendererConfig& config);
        void shutdown();
        void resize(std::uint32_t width, std::uint32_t height);

        void beginFrame();
        void setFrameStats(float deltaSeconds);
        void beginScene(const Camera& camera);
        void setDirectionalLight(const DirectionalLight& light);
        void drawSkybox(const Skybox& skybox);
        void drawGround(float size, const Material& material);
        void drawBlobShadow(const Transform& casterTransform, float groundY, float opacity);
        void drawCube(const Transform& transform, const Material& material);
        void drawSphere(const Transform& transform, const Material& material);
        void drawSun(const DirectionalLight& light, float distance, float size);
        void endFrame();

        [[nodiscard]] std::uint32_t width() const;
        [[nodiscard]] std::uint32_t height() const;
        [[nodiscard]] const RenderStats& stats() const;

    private:
        struct Handles;

        void createGeometry();
        void createPrograms();
        void destroyGeometry();
        void destroyPrograms();
        void setObjectTransform(const Transform& transform);
        void setPbrUniforms(const Material& material);
        [[nodiscard]] std::string shaderPath(const char* shaderName) const;

        Handles* m_handles = nullptr;
        std::string m_shaderDirectory;
        std::uint32_t m_width = 1;
        std::uint32_t m_height = 1;
        Camera m_activeCamera{};
        DirectionalLight m_light{};
        float m_fps = 0.0f;
        float m_frameTimeAccumulator = 0.0f;
        std::uint32_t m_frameCounter = 0;
        RenderStats m_stats{};
        bool m_sceneActive = false;
        bool m_initialized = false;
    };
}
