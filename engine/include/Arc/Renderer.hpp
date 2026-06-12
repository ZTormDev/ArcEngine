#pragma once

#include "Arc/Camera.hpp"
#include "Arc/Math.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace Arc
{
    struct MeshData;

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

    struct TextureHandle
    {
        std::uint32_t id = UINT32_MAX;

        [[nodiscard]] bool isValid() const
        {
            return id != UINT32_MAX;
        }
    };

    struct TextureSlot
    {
        TextureHandle handle{};
        std::string path;
        std::vector<std::uint8_t> encodedData;
        std::string debugName;
        bool srgb = false;
    };

    struct Material
    {
        Color baseColor{ 1.0f, 1.0f, 1.0f, 1.0f };
        float metallic = 0.0f;
        float roughness = 0.55f;
        float emissive = 0.0f;
        TextureSlot albedoTexture{ {}, {}, {}, {}, true };
        TextureSlot normalTexture{};
        TextureSlot metallicRoughnessTexture{};
        TextureSlot aoTexture{};
        TextureSlot emissiveTexture{ {}, {}, {}, {}, true };
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
    };

    struct MeshHandle
    {
        std::uint32_t id = UINT32_MAX;

        [[nodiscard]] bool isValid() const
        {
            return id != UINT32_MAX;
        }
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
        [[nodiscard]] TextureHandle loadTexture(const std::string& path, bool srgb = false);
        [[nodiscard]] TextureHandle loadTextureFromMemory(const std::vector<std::uint8_t>& encodedData, const std::string& debugName, bool srgb = false);
        void loadMaterialTextures(Material& material);
        void destroyTexture(TextureHandle handle);
        [[nodiscard]] MeshHandle createMesh(const MeshData& meshData);
        void destroyMesh(MeshHandle handle);
        void beginScene(const Camera& camera);
        void setDirectionalLight(const DirectionalLight& light);
        void drawSkybox(const Skybox& skybox);
        void drawGround(float size, const Material& material);
        void drawCube(const Transform& transform, const Transform& prevTransform, const Material& material);
        void drawSphere(const Transform& transform, const Transform& prevTransform, const Material& material);
        void drawMesh(MeshHandle mesh, const Transform& transform, const Transform& prevTransform, const Material& material);
        void drawSun(const DirectionalLight& light, float distance, float size);
        void endFrame();

        [[nodiscard]] std::uint32_t width() const;
        [[nodiscard]] std::uint32_t height() const;
        [[nodiscard]] const RenderStats& stats() const;

        void setPostProcessingMode(int mode) { m_postProcessMode = mode; }
        [[nodiscard]] int postProcessingMode() const { return m_postProcessMode; }

        void setExposure(float exposure) { m_exposure = exposure; }
        [[nodiscard]] float exposure() const { return m_exposure; }

        void setBloomIntensity(float intensity) { m_bloomIntensity = intensity; }
        [[nodiscard]] float bloomIntensity() const { return m_bloomIntensity; }

        void setBloomThreshold(float threshold) { m_bloomThreshold = threshold; }
        [[nodiscard]] float bloomThreshold() const { return m_bloomThreshold; }

        void setBloomEnabled(bool enabled) { m_bloomEnabled = enabled; }
        [[nodiscard]] bool bloomEnabled() const { return m_bloomEnabled; }

        void setTaaEnabled(bool enabled) { m_taaEnabled = enabled; }
        [[nodiscard]] bool taaEnabled() const { return m_taaEnabled; }

        void setDofEnabled(bool enabled) { m_dofEnabled = enabled; }
        [[nodiscard]] bool dofEnabled() const { return m_dofEnabled; }

        void setMotionBlurEnabled(bool enabled) { m_motionBlurEnabled = enabled; }
        [[nodiscard]] bool motionBlurEnabled() const { return m_motionBlurEnabled; }

        void setAutoExposureEnabled(bool enabled) { m_autoExposureEnabled = enabled; }
        [[nodiscard]] bool autoExposureEnabled() const { return m_autoExposureEnabled; }

    private:
        struct Handles;

        void createGeometry();
        void createPrograms();
        [[nodiscard]] TextureHandle createTextureFromRgbaPixels(const void* pixels, std::uint16_t width, std::uint16_t height, bool srgb);
        void createDefaultTextures();
        void destroyGeometry();
        void destroyPrograms();
        void setObjectTransform(const Transform& transform);
        void setPrevObjectTransform(const Transform& transform);
        void setPbrUniforms(const Material& material);
        void bindShadowUniformsAndTextures();
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
        float m_shadowMatrices[4 * 16] = { 0.0f };
        float m_cascadeSplits[4] = { 0.0f };
        bool m_sceneActive = false;
        bool m_initialized = false;
        int m_postProcessMode = 0; // 0 = Full HDR, 1 = Raw bypass, 2 = LDR
        float m_exposure = 1.15f;
        float m_bloomIntensity = 0.25f;
        float m_bloomThreshold = 0.5f;
        float m_currentViewProj[16] = { 0.0f };
        float m_prevViewProj[16] = { 0.0f };
        std::uint32_t m_jitterIndex = 0;
        float m_jitterX = 0.0f;
        float m_jitterY = 0.0f;
        float m_prevJitterX = 0.0f;
        float m_prevJitterY = 0.0f;
        bool m_bloomEnabled = false;
        bool m_taaEnabled = true;
        bool m_dofEnabled = false;
        bool m_motionBlurEnabled = true;
        bool m_autoExposureEnabled = false;
        bool m_tonemapEnabled = false;
        std::uint32_t m_taaHistoryIndex = 0;
        std::uint32_t m_luminanceIndex = 0;
        float m_deltaTime = 0.016f;
    };
}
