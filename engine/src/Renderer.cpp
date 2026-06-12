#include "Arc/Renderer.hpp"

#include "Arc/Mesh.hpp"

#include <bgfx/bgfx.h>
#include <bx/math.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace Arc
{
    constexpr bgfx::ViewId ViewIdShadow0 = 0;
    constexpr bgfx::ViewId ViewIdShadow1 = 1;
    constexpr bgfx::ViewId ViewIdShadow2 = 2;
    constexpr bgfx::ViewId ViewIdShadow3 = 3;
    constexpr bgfx::ViewId ViewIdSkybox  = 4;
    constexpr bgfx::ViewId ViewIdScene   = 5;
    constexpr bgfx::ViewId ViewIdBloomDown0 = 6;
    constexpr bgfx::ViewId ViewIdBloomDown1 = 7;
    constexpr bgfx::ViewId ViewIdBloomDown2 = 8;
    constexpr bgfx::ViewId ViewIdBloomDown3 = 9;
    constexpr bgfx::ViewId ViewIdBloomUp0   = 10;
    constexpr bgfx::ViewId ViewIdBloomUp1   = 11;
    constexpr bgfx::ViewId ViewIdBloomUp2   = 12;
    constexpr bgfx::ViewId ViewIdDepthOfField = 13;
    constexpr bgfx::ViewId ViewIdTAA = 14;
    constexpr bgfx::ViewId ViewIdMotionBlur = 15;
    constexpr bgfx::ViewId ViewIdLuminanceDown0 = 16;
    constexpr bgfx::ViewId ViewIdLuminanceDown1 = 17;
    constexpr bgfx::ViewId ViewIdLuminanceDown2 = 18;
    constexpr bgfx::ViewId ViewIdLuminanceDown3 = 19;
    constexpr bgfx::ViewId ViewIdLuminanceDown4 = 20;
    constexpr bgfx::ViewId ViewIdLuminanceAdapt = 21;
    constexpr bgfx::ViewId ViewIdPostProcess = 22;
    constexpr bgfx::ViewId ViewCount       = 23;

    namespace
    {
        struct PosNormalColorVertex
        {
            float x;
            float y;
            float z;
            float nx;
            float ny;
            float nz;
            std::uint32_t abgr;
            float u = 0.0f;
            float v = 0.0f;
            float tx = 1.0f;
            float ty = 0.0f;
            float tz = 0.0f;
            float tw = 1.0f;

            static bgfx::VertexLayout layout;
        };

        bgfx::VertexLayout PosNormalColorVertex::layout;

        struct PostProcessVertex
        {
            float x;
            float y;
            float z;
            float u;
            float v;

            static bgfx::VertexLayout layout;
        };

        bgfx::VertexLayout PostProcessVertex::layout;

        void renderFullScreenQuad(bgfx::ViewId viewId, bgfx::ProgramHandle program)
        {
            bgfx::TransientVertexBuffer tvb;
            bgfx::allocTransientVertexBuffer(&tvb, 3, PostProcessVertex::layout);
            if (tvb.data == nullptr)
            {
                return;
            }

            PostProcessVertex* vertices = reinterpret_cast<PostProcessVertex*>(tvb.data);

            bool originBottomLeft = bgfx::getCaps()->originBottomLeft;

            vertices[0].x = -1.0f;
            vertices[0].y = -1.0f;
            vertices[0].z = 0.0f;
            vertices[0].u = 0.0f;
            vertices[0].v = originBottomLeft ? 0.0f : 1.0f;

            vertices[1].x = 3.0f;
            vertices[1].y = -1.0f;
            vertices[1].z = 0.0f;
            vertices[1].u = 2.0f;
            vertices[1].v = originBottomLeft ? 0.0f : 1.0f;

            vertices[2].x = -1.0f;
            vertices[2].y = 3.0f;
            vertices[2].z = 0.0f;
            vertices[2].u = 0.0f;
            vertices[2].v = originBottomLeft ? 2.0f : -1.0f;

            bgfx::setVertexBuffer(0, &tvb);
            bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_ALWAYS);
            bgfx::submit(viewId, program);
        }

        constexpr std::uint32_t White = 0xffffffffu;
        constexpr std::uint32_t DefaultTextureId = UINT32_MAX;

        const PosNormalColorVertex CubeVertices[] = {
            { -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, White },
            {  1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, White },
            {  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, White },
            { -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, White },
            {  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, White },
            { -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, White },
            { -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, White },
            {  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, White },
            { -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, White },
            { -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, White },
            { -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, White },
            { -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, White },
            {  1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, White },
            {  1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, White },
            {  1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, White },
            {  1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, White },
            { -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, White },
            {  1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, White },
            {  1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, White },
            { -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, White },
            { -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, White },
            {  1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, White },
            {  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, White },
            { -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, White },
        };

        const std::uint16_t CubeIndices[] = {
             0,  1,  2,  0,  2,  3,
             4,  5,  6,  4,  6,  7,
             8,  9, 10,  8, 10, 11,
            12, 13, 14, 12, 14, 15,
            16, 17, 18, 16, 18, 19,
            20, 21, 22, 20, 22, 23,
        };

        


        const PosNormalColorVertex PlaneVertices[] = {
            { -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, White },
            {  1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, White },
            {  1.0f, 0.0f,  1.0f, 0.0f, 1.0f, 0.0f, White },
            { -1.0f, 0.0f,  1.0f, 0.0f, 1.0f, 0.0f, White },
        };

        const std::uint16_t PlaneIndices[] = {
            0, 1, 2, 0, 2, 3,
        };

        void buildSphere(std::vector<PosNormalColorVertex>& vertices, std::vector<std::uint16_t>& indices)
        {
            constexpr int rings = 16;
            constexpr int segments = 32;

            vertices.reserve((rings + 1) * (segments + 1));
            indices.reserve(rings * segments * 6);

            for(int ring = 0; ring <= rings; ++ring)
            {
                const float v = static_cast<float>(ring) / static_cast<float>(rings);
                const float theta = v * Pi;
                const float y = std::cos(theta);
                const float radius = std::sin(theta);

                for(int segment = 0; segment <= segments; ++segment)
                {
                    const float u = static_cast<float>(segment) / static_cast<float>(segments);
                    const float phi = u * Pi * 2.0f;
                    const float x = std::sin(phi) * radius;
                    const float z = std::cos(phi) * radius;

                    vertices.push_back({ x, y, z, x, y, z, White });
                }
            }

            for(int ring = 0; ring < rings; ++ring)
            {
                for(int segment = 0; segment < segments; ++segment)
                {
                    const auto current = static_cast<std::uint16_t>(ring * (segments + 1) + segment);
                    const auto next = static_cast<std::uint16_t>(current + segments + 1);

                    indices.push_back(current);
                    indices.push_back(next);
                    indices.push_back(static_cast<std::uint16_t>(current + 1));
                    indices.push_back(static_cast<std::uint16_t>(current + 1));
                    indices.push_back(next);
                    indices.push_back(static_cast<std::uint16_t>(next + 1));
                }
            }
        }

        void buildDisk(std::vector<PosNormalColorVertex>& vertices, std::vector<std::uint16_t>& indices)
        {
            constexpr int segments = 48;

            vertices.reserve(segments + 2);
            indices.reserve(segments * 3);
            vertices.push_back({ 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, White });

            for(int segment = 0; segment <= segments; ++segment)
            {
                const float angle = static_cast<float>(segment) / static_cast<float>(segments) * Pi * 2.0f;
                vertices.push_back({ std::sin(angle), 0.0f, std::cos(angle), 0.0f, 1.0f, 0.0f, White });
            }

            for(int segment = 1; segment <= segments; ++segment)
            {
                indices.push_back(0);
                indices.push_back(static_cast<std::uint16_t>(segment));
                indices.push_back(static_cast<std::uint16_t>(segment + 1));
            }
        }

        [[nodiscard]] bool valid(bgfx::ProgramHandle handle)
        {
            return bgfx::isValid(handle);
        }

        [[nodiscard]] bool valid(bgfx::UniformHandle handle)
        {
            return bgfx::isValid(handle);
        }

        [[nodiscard]] bgfx::ShaderHandle loadShader(const std::string& path)
        {
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if(!file)
            {
                throw std::runtime_error("Failed to open shader: " + path);
            }

            const std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            const bgfx::Memory* memory = bgfx::alloc(static_cast<std::uint32_t>(size + 1));
            if(!file.read(reinterpret_cast<char*>(memory->data), size))
            {
                throw std::runtime_error("Failed to read shader: " + path);
            }

            memory->data[memory->size - 1] = '\0';
            return bgfx::createShader(memory);
        }

        [[nodiscard]] bgfx::ProgramHandle loadProgram(const std::string& vertexPath, const std::string& fragmentPath)
        {
            bgfx::ShaderHandle vertexShader = loadShader(vertexPath);
            bgfx::ShaderHandle fragmentShader = loadShader(fragmentPath);
            return bgfx::createProgram(vertexShader, fragmentShader, true);
        }
    }

    struct Renderer::Handles
    {
        Handles()
        {
            for (int i = 0; i < 4; ++i)
            {
                bloomDownTextures[i] = BGFX_INVALID_HANDLE;
                bloomDownFrameBuffers[i] = BGFX_INVALID_HANDLE;
            }
            for (int i = 0; i < 3; ++i)
            {
                bloomUpTextures[i] = BGFX_INVALID_HANDLE;
                bloomUpFrameBuffers[i] = BGFX_INVALID_HANDLE;
            }
            for (int i = 0; i < 2; ++i)
            {
                taaHistoryTextures[i] = BGFX_INVALID_HANDLE;
                taaHistoryFrameBuffers[i] = BGFX_INVALID_HANDLE;
                luminanceAdaptTextures[i] = BGFX_INVALID_HANDLE;
                luminanceAdaptFrameBuffers[i] = BGFX_INVALID_HANDLE;
            }
            for (int i = 0; i < 5; ++i)
            {
                luminanceDownTextures[i] = BGFX_INVALID_HANDLE;
                luminanceDownFrameBuffers[i] = BGFX_INVALID_HANDLE;
            }
        }

        struct GpuMesh
        {
            bgfx::VertexBufferHandle vertexBuffer = BGFX_INVALID_HANDLE;
            bgfx::IndexBufferHandle indexBuffer = BGFX_INVALID_HANDLE;
            bool alive = false;
        };

        struct GpuTexture
        {
            bgfx::TextureHandle texture = BGFX_INVALID_HANDLE;
            bool alive = false;
        };

        bgfx::VertexBufferHandle cubeVertexBuffer = BGFX_INVALID_HANDLE;
        bgfx::IndexBufferHandle cubeIndexBuffer = BGFX_INVALID_HANDLE;
        bgfx::VertexBufferHandle planeVertexBuffer = BGFX_INVALID_HANDLE;
        bgfx::IndexBufferHandle planeIndexBuffer = BGFX_INVALID_HANDLE;
        bgfx::VertexBufferHandle sphereVertexBuffer = BGFX_INVALID_HANDLE;
        bgfx::IndexBufferHandle sphereIndexBuffer = BGFX_INVALID_HANDLE;
        bgfx::VertexBufferHandle diskVertexBuffer = BGFX_INVALID_HANDLE;
        bgfx::IndexBufferHandle diskIndexBuffer = BGFX_INVALID_HANDLE;
        bgfx::ProgramHandle meshProgram = BGFX_INVALID_HANDLE;
        bgfx::ProgramHandle skyProgram = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle lightDirection = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle lightColor = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle cameraData = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle ambientColor = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle tint = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle material = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle albedoSampler = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle normalSampler = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle metallicRoughnessSampler = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle aoSampler = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle emissiveSampler = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle skyHorizon = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle skyZenith = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle skySunGlow = BGFX_INVALID_HANDLE;
        bgfx::TextureHandle shadowTexture = BGFX_INVALID_HANDLE;
        bgfx::FrameBufferHandle shadowFrameBuffer = BGFX_INVALID_HANDLE;
        bgfx::ProgramHandle shadowProgram = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle shadowMtxUniform = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle shadowMapSampler = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle cameraForwardUniform = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle cascadeSplitsUniform = BGFX_INVALID_HANDLE;
        
        // HDR Scene Framebuffer & Velocity (MRT)
        bgfx::TextureHandle hdrColorTexture = BGFX_INVALID_HANDLE;
        bgfx::TextureHandle velocityTexture = BGFX_INVALID_HANDLE;
        bgfx::TextureHandle hdrDepthTexture = BGFX_INVALID_HANDLE;
        bgfx::FrameBufferHandle hdrFrameBuffer = BGFX_INVALID_HANDLE;

        // Bloom
        bgfx::TextureHandle bloomDownTextures[4];
        bgfx::FrameBufferHandle bloomDownFrameBuffers[4];
        bgfx::TextureHandle bloomUpTextures[3];
        bgfx::FrameBufferHandle bloomUpFrameBuffers[3];
        bgfx::ProgramHandle bloomDownProgram = BGFX_INVALID_HANDLE;
        bgfx::ProgramHandle bloomUpProgram = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle downParamsUniform = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle upParamsUniform = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle textureSampler = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle lowTexSampler = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle highTexSampler = BGFX_INVALID_HANDLE;

        // TAA
        bgfx::TextureHandle taaHistoryTextures[2];
        bgfx::FrameBufferHandle taaHistoryFrameBuffers[2];
        bgfx::ProgramHandle taaProgram = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle taaParamsUniform = BGFX_INVALID_HANDLE;
        
        // Depth of Field
        bgfx::TextureHandle dofTexture = BGFX_INVALID_HANDLE;
        bgfx::FrameBufferHandle dofFrameBuffer = BGFX_INVALID_HANDLE;
        bgfx::ProgramHandle dofProgram = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle dofParamsUniform = BGFX_INVALID_HANDLE;

        // Motion Blur
        bgfx::TextureHandle motionBlurTexture = BGFX_INVALID_HANDLE;
        bgfx::FrameBufferHandle motionBlurFrameBuffer = BGFX_INVALID_HANDLE;
        bgfx::ProgramHandle motionBlurProgram = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle motionBlurParamsUniform = BGFX_INVALID_HANDLE;

        // Auto Exposure / Eye Adaptation
        bgfx::TextureHandle luminanceDownTextures[5];
        bgfx::FrameBufferHandle luminanceDownFrameBuffers[5];
        bgfx::ProgramHandle luminanceDownProgram = BGFX_INVALID_HANDLE;
        bgfx::TextureHandle luminanceAdaptTextures[2];
        bgfx::FrameBufferHandle luminanceAdaptFrameBuffers[2];
        bgfx::ProgramHandle luminanceAdaptProgram = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle adaptationParamsUniform = BGFX_INVALID_HANDLE;

        // Final Post
        bgfx::ProgramHandle postProgram = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle postParamsUniform = BGFX_INVALID_HANDLE;

        // Shared Post-Process Samplers
        bgfx::UniformHandle sceneTexSampler = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle bloomTexSampler = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle currentTexSampler = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle historyTexSampler = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle velocityTexSampler = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle depthTexSampler = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle luminanceTexSampler = BGFX_INVALID_HANDLE;

        // Velocity tracking uniforms
        bgfx::UniformHandle prevModelUniform = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle prevViewProjUniform = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle currentViewProjUniform = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle taaJitterUniform = BGFX_INVALID_HANDLE;


        std::vector<GpuMesh> gpuMeshes;
        std::vector<GpuTexture> gpuTextures;
        std::unordered_map<std::string, TextureHandle> textureCache;
        TextureHandle defaultWhiteTexture{};
        TextureHandle defaultBlackTexture{};
        TextureHandle defaultNormalTexture{};
    };

    Renderer::~Renderer()
    {
        shutdown();
    }

    void Renderer::initialize(const RendererConfig& config)
    {
        m_width = config.width;
        m_height = config.height;
        m_shaderDirectory = config.shaderDirectory;
        m_handles = new Handles();

        PosNormalColorVertex::layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Tangent, 4, bgfx::AttribType::Float)
            .end();

        PostProcessVertex::layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .end();

        createGeometry();
        createDefaultTextures();
        createPrograms();

        // Create 8192x8192 shadow depth texture and framebuffer
        std::uint64_t shadowTextureFlags = BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_COMPARE_LEQUAL;
        m_handles->shadowTexture = bgfx::createTexture2D(
            8192,
            8192,
            false,
            1,
            bgfx::TextureFormat::D32F,
            shadowTextureFlags
        );

        bgfx::Attachment shadowAttachment;
        shadowAttachment.init(m_handles->shadowTexture, bgfx::Access::Write);
        m_handles->shadowFrameBuffer = bgfx::createFrameBuffer(1, &shadowAttachment, true);

        bgfx::setDebug(BGFX_DEBUG_TEXT);
        resize(m_width, m_height);
        m_initialized = true;
    }

    void Renderer::shutdown()
    {
        if(!m_initialized && m_handles == nullptr)
        {
            return;
        }

        if(bgfx::isValid(m_handles->shadowFrameBuffer))
        {
            bgfx::destroy(m_handles->shadowFrameBuffer);
            m_handles->shadowFrameBuffer = BGFX_INVALID_HANDLE;
            m_handles->shadowTexture = BGFX_INVALID_HANDLE;
        }

        if(bgfx::isValid(m_handles->hdrFrameBuffer))
        {
            bgfx::destroy(m_handles->hdrFrameBuffer);
            m_handles->hdrFrameBuffer = BGFX_INVALID_HANDLE;
            m_handles->hdrColorTexture = BGFX_INVALID_HANDLE;
            m_handles->velocityTexture = BGFX_INVALID_HANDLE;
            m_handles->hdrDepthTexture = BGFX_INVALID_HANDLE;
        }

        for (int i = 0; i < 4; ++i)
        {
            if (bgfx::isValid(m_handles->bloomDownFrameBuffers[i]))
            {
                bgfx::destroy(m_handles->bloomDownFrameBuffers[i]);
                m_handles->bloomDownFrameBuffers[i] = BGFX_INVALID_HANDLE;
                m_handles->bloomDownTextures[i] = BGFX_INVALID_HANDLE;
            }
        }

        for (int i = 0; i < 3; ++i)
        {
            if (bgfx::isValid(m_handles->bloomUpFrameBuffers[i]))
            {
                bgfx::destroy(m_handles->bloomUpFrameBuffers[i]);
                m_handles->bloomUpFrameBuffers[i] = BGFX_INVALID_HANDLE;
                m_handles->bloomUpTextures[i] = BGFX_INVALID_HANDLE;
            }
        }

        for (int i = 0; i < 2; ++i)
        {
            if (bgfx::isValid(m_handles->taaHistoryFrameBuffers[i]))
            {
                bgfx::destroy(m_handles->taaHistoryFrameBuffers[i]);
                m_handles->taaHistoryFrameBuffers[i] = BGFX_INVALID_HANDLE;
                m_handles->taaHistoryTextures[i] = BGFX_INVALID_HANDLE;
            }
            if (bgfx::isValid(m_handles->luminanceAdaptFrameBuffers[i]))
            {
                bgfx::destroy(m_handles->luminanceAdaptFrameBuffers[i]);
                m_handles->luminanceAdaptFrameBuffers[i] = BGFX_INVALID_HANDLE;
                m_handles->luminanceAdaptTextures[i] = BGFX_INVALID_HANDLE;
            }
        }

        if (bgfx::isValid(m_handles->dofFrameBuffer))
        {
            bgfx::destroy(m_handles->dofFrameBuffer);
            m_handles->dofFrameBuffer = BGFX_INVALID_HANDLE;
            m_handles->dofTexture = BGFX_INVALID_HANDLE;
        }

        if (bgfx::isValid(m_handles->motionBlurFrameBuffer))
        {
            bgfx::destroy(m_handles->motionBlurFrameBuffer);
            m_handles->motionBlurFrameBuffer = BGFX_INVALID_HANDLE;
            m_handles->motionBlurTexture = BGFX_INVALID_HANDLE;
        }

        for (int i = 0; i < 5; ++i)
        {
            if (bgfx::isValid(m_handles->luminanceDownFrameBuffers[i]))
            {
                bgfx::destroy(m_handles->luminanceDownFrameBuffers[i]);
                m_handles->luminanceDownFrameBuffers[i] = BGFX_INVALID_HANDLE;
                m_handles->luminanceDownTextures[i] = BGFX_INVALID_HANDLE;
            }
        }

        destroyPrograms();
        destroyGeometry();

        delete m_handles;
        m_handles = nullptr;
        m_initialized = false;
    }

    void Renderer::resize(std::uint32_t width, std::uint32_t height)
    {
        m_width = width > 0 ? width : 1;
        m_height = height > 0 ? height : 1;

        // Configure the 4 shadow cascade views in a 2x2 grid in the 8192x8192 framebuffer
        for (bgfx::ViewId i = 0; i < 4; ++i)
        {
            bgfx::setViewFrameBuffer(i, m_handles->shadowFrameBuffer);
            std::uint16_t x = (i % 2) * 4096;
            std::uint16_t y = (i / 2) * 4096;
            bgfx::setViewRect(i, x, y, 4096, 4096);
        }

        // Configure clears: clear the depth buffer for all shadow cascade quadrants
        bgfx::setViewClear(ViewIdShadow0, BGFX_CLEAR_DEPTH, 0, 1.0f, 0);
        bgfx::setViewClear(ViewIdShadow1, BGFX_CLEAR_DEPTH, 0, 1.0f, 0);
        bgfx::setViewClear(ViewIdShadow2, BGFX_CLEAR_DEPTH, 0, 1.0f, 0);
        bgfx::setViewClear(ViewIdShadow3, BGFX_CLEAR_DEPTH, 0, 1.0f, 0);

        // Destroy existing resources if valid
        if(bgfx::isValid(m_handles->hdrFrameBuffer))
        {
            bgfx::destroy(m_handles->hdrFrameBuffer);
            m_handles->hdrFrameBuffer = BGFX_INVALID_HANDLE;
            m_handles->hdrColorTexture = BGFX_INVALID_HANDLE;
            m_handles->velocityTexture = BGFX_INVALID_HANDLE;
            m_handles->hdrDepthTexture = BGFX_INVALID_HANDLE;
        }

        for (int i = 0; i < 4; ++i)
        {
            if (bgfx::isValid(m_handles->bloomDownFrameBuffers[i]))
            {
                bgfx::destroy(m_handles->bloomDownFrameBuffers[i]);
                m_handles->bloomDownFrameBuffers[i] = BGFX_INVALID_HANDLE;
                m_handles->bloomDownTextures[i] = BGFX_INVALID_HANDLE;
            }
        }

        for (int i = 0; i < 3; ++i)
        {
            if (bgfx::isValid(m_handles->bloomUpFrameBuffers[i]))
            {
                bgfx::destroy(m_handles->bloomUpFrameBuffers[i]);
                m_handles->bloomUpFrameBuffers[i] = BGFX_INVALID_HANDLE;
                m_handles->bloomUpTextures[i] = BGFX_INVALID_HANDLE;
            }
        }

        for (int i = 0; i < 2; ++i)
        {
            if (bgfx::isValid(m_handles->taaHistoryFrameBuffers[i]))
            {
                bgfx::destroy(m_handles->taaHistoryFrameBuffers[i]);
                m_handles->taaHistoryFrameBuffers[i] = BGFX_INVALID_HANDLE;
                m_handles->taaHistoryTextures[i] = BGFX_INVALID_HANDLE;
            }
            if (bgfx::isValid(m_handles->luminanceAdaptFrameBuffers[i]))
            {
                bgfx::destroy(m_handles->luminanceAdaptFrameBuffers[i]);
                m_handles->luminanceAdaptFrameBuffers[i] = BGFX_INVALID_HANDLE;
                m_handles->luminanceAdaptTextures[i] = BGFX_INVALID_HANDLE;
            }
        }

        if (bgfx::isValid(m_handles->dofFrameBuffer))
        {
            bgfx::destroy(m_handles->dofFrameBuffer);
            m_handles->dofFrameBuffer = BGFX_INVALID_HANDLE;
            m_handles->dofTexture = BGFX_INVALID_HANDLE;
        }

        if (bgfx::isValid(m_handles->motionBlurFrameBuffer))
        {
            bgfx::destroy(m_handles->motionBlurFrameBuffer);
            m_handles->motionBlurFrameBuffer = BGFX_INVALID_HANDLE;
            m_handles->motionBlurTexture = BGFX_INVALID_HANDLE;
        }

        for (int i = 0; i < 5; ++i)
        {
            if (bgfx::isValid(m_handles->luminanceDownFrameBuffers[i]))
            {
                bgfx::destroy(m_handles->luminanceDownFrameBuffers[i]);
                m_handles->luminanceDownFrameBuffers[i] = BGFX_INVALID_HANDLE;
                m_handles->luminanceDownTextures[i] = BGFX_INVALID_HANDLE;
            }
        }

        // 1. Create HDR Framebuffer & textures (MRT: color + velocity + depth)
        std::uint64_t rtFlags = BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
        m_handles->hdrColorTexture = bgfx::createTexture2D(
            static_cast<std::uint16_t>(m_width),
            static_cast<std::uint16_t>(m_height),
            false,
            1,
            bgfx::TextureFormat::RGBA16F,
            rtFlags
        );
        m_handles->velocityTexture = bgfx::createTexture2D(
            static_cast<std::uint16_t>(m_width),
            static_cast<std::uint16_t>(m_height),
            false,
            1,
            bgfx::TextureFormat::RGBA16F,
            rtFlags
        );
        m_handles->hdrDepthTexture = bgfx::createTexture2D(
            static_cast<std::uint16_t>(m_width),
            static_cast<std::uint16_t>(m_height),
            false,
            1,
            bgfx::TextureFormat::D32F,
            rtFlags
        );

        bgfx::Attachment hdrAttachments[3];
        hdrAttachments[0].init(m_handles->hdrColorTexture, bgfx::Access::Write);
        hdrAttachments[1].init(m_handles->velocityTexture, bgfx::Access::Write);
        hdrAttachments[2].init(m_handles->hdrDepthTexture, bgfx::Access::Write);
        m_handles->hdrFrameBuffer = bgfx::createFrameBuffer(3, hdrAttachments, true);

        // 2. Create Bloom Downsample levels (W/2, W/4, W/8, W/16)
        std::uint16_t w = static_cast<std::uint16_t>(m_width);
        std::uint16_t h = static_cast<std::uint16_t>(m_height);
        for (int i = 0; i < 4; ++i)
        {
            w = std::max<std::uint16_t>(1, w / 2);
            h = std::max<std::uint16_t>(1, h / 2);
            m_handles->bloomDownTextures[i] = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::RGBA16F, rtFlags);
            bgfx::Attachment attachment;
            attachment.init(m_handles->bloomDownTextures[i], bgfx::Access::Write);
            m_handles->bloomDownFrameBuffers[i] = bgfx::createFrameBuffer(1, &attachment, true);

            // Configure views for downsample
            bgfx::ViewId viewId = static_cast<bgfx::ViewId>(ViewIdBloomDown0 + i);
            bgfx::setViewFrameBuffer(viewId, m_handles->bloomDownFrameBuffers[i]);
            bgfx::setViewRect(viewId, 0, 0, w, h);
            bgfx::setViewClear(viewId, BGFX_CLEAR_NONE, 0, 1.0f, 0);
        }

        // 3. Create Bloom Upsample levels
        std::uint16_t upSizes[3][2];
        upSizes[0][0] = std::max<std::uint16_t>(1, static_cast<std::uint16_t>(m_width) / 8);
        upSizes[0][1] = std::max<std::uint16_t>(1, static_cast<std::uint16_t>(m_height) / 8);
        upSizes[1][0] = std::max<std::uint16_t>(1, static_cast<std::uint16_t>(m_width) / 4);
        upSizes[1][1] = std::max<std::uint16_t>(1, static_cast<std::uint16_t>(m_height) / 4);
        upSizes[2][0] = std::max<std::uint16_t>(1, static_cast<std::uint16_t>(m_width) / 2);
        upSizes[2][1] = std::max<std::uint16_t>(1, static_cast<std::uint16_t>(m_height) / 2);

        for (int i = 0; i < 3; ++i)
        {
            m_handles->bloomUpTextures[i] = bgfx::createTexture2D(upSizes[i][0], upSizes[i][1], false, 1, bgfx::TextureFormat::RGBA16F, rtFlags);
            bgfx::Attachment attachment;
            attachment.init(m_handles->bloomUpTextures[i], bgfx::Access::Write);
            m_handles->bloomUpFrameBuffers[i] = bgfx::createFrameBuffer(1, &attachment, true);

            // Configure views for upsample
            bgfx::ViewId viewId = static_cast<bgfx::ViewId>(ViewIdBloomUp0 + i);
            bgfx::setViewFrameBuffer(viewId, m_handles->bloomUpFrameBuffers[i]);
            bgfx::setViewRect(viewId, 0, 0, upSizes[i][0], upSizes[i][1]);
            bgfx::setViewClear(viewId, BGFX_CLEAR_NONE, 0, 1.0f, 0);
        }

        // 4. Create TAA double buffers
        for (int i = 0; i < 2; ++i)
        {
            m_handles->taaHistoryTextures[i] = bgfx::createTexture2D(
                static_cast<std::uint16_t>(m_width),
                static_cast<std::uint16_t>(m_height),
                false,
                1,
                bgfx::TextureFormat::RGBA16F,
                rtFlags
            );
            bgfx::Attachment attachment;
            attachment.init(m_handles->taaHistoryTextures[i], bgfx::Access::Write);
            m_handles->taaHistoryFrameBuffers[i] = bgfx::createFrameBuffer(1, &attachment, true);
        }
        bgfx::setViewRect(ViewIdTAA, 0, 0, static_cast<std::uint16_t>(m_width), static_cast<std::uint16_t>(m_height));
        bgfx::setViewClear(ViewIdTAA, BGFX_CLEAR_NONE, 0, 1.0f, 0);

        // 5. Create DoF framebuffer
        m_handles->dofTexture = bgfx::createTexture2D(
            static_cast<std::uint16_t>(m_width),
            static_cast<std::uint16_t>(m_height),
            false,
            1,
            bgfx::TextureFormat::RGBA16F,
            rtFlags
        );
        bgfx::Attachment dofAttachment;
        dofAttachment.init(m_handles->dofTexture, bgfx::Access::Write);
        m_handles->dofFrameBuffer = bgfx::createFrameBuffer(1, &dofAttachment, true);
        bgfx::setViewFrameBuffer(ViewIdDepthOfField, m_handles->dofFrameBuffer);
        bgfx::setViewRect(ViewIdDepthOfField, 0, 0, static_cast<std::uint16_t>(m_width), static_cast<std::uint16_t>(m_height));
        bgfx::setViewClear(ViewIdDepthOfField, BGFX_CLEAR_NONE, 0, 1.0f, 0);

        // 6. Create Motion Blur framebuffer
        m_handles->motionBlurTexture = bgfx::createTexture2D(
            static_cast<std::uint16_t>(m_width),
            static_cast<std::uint16_t>(m_height),
            false,
            1,
            bgfx::TextureFormat::RGBA16F,
            rtFlags
        );
        bgfx::Attachment mbAttachment;
        mbAttachment.init(m_handles->motionBlurTexture, bgfx::Access::Write);
        m_handles->motionBlurFrameBuffer = bgfx::createFrameBuffer(1, &mbAttachment, true);
        bgfx::setViewFrameBuffer(ViewIdMotionBlur, m_handles->motionBlurFrameBuffer);
        bgfx::setViewRect(ViewIdMotionBlur, 0, 0, static_cast<std::uint16_t>(m_width), static_cast<std::uint16_t>(m_height));
        bgfx::setViewClear(ViewIdMotionBlur, BGFX_CLEAR_NONE, 0, 1.0f, 0);

        // 7. Create Luminance Downsample chain (256x256, 64x64, 16x16, 4x4, 1x1)
        std::uint16_t lumSize = 256;
        for (int i = 0; i < 5; ++i)
        {
            m_handles->luminanceDownTextures[i] = bgfx::createTexture2D(
                lumSize,
                lumSize,
                false,
                1,
                bgfx::TextureFormat::RGBA16F,
                rtFlags
            );
            bgfx::Attachment attachment;
            attachment.init(m_handles->luminanceDownTextures[i], bgfx::Access::Write);
            m_handles->luminanceDownFrameBuffers[i] = bgfx::createFrameBuffer(1, &attachment, true);

            bgfx::ViewId viewId = static_cast<bgfx::ViewId>(ViewIdLuminanceDown0 + i);
            bgfx::setViewFrameBuffer(viewId, m_handles->luminanceDownFrameBuffers[i]);
            bgfx::setViewRect(viewId, 0, 0, lumSize, lumSize);
            bgfx::setViewClear(viewId, BGFX_CLEAR_NONE, 0, 1.0f, 0);

            lumSize = std::max<std::uint16_t>(1, lumSize / 4);
        }

        // 8. Create Luminance Adaptation double buffers (1x1)
        for (int i = 0; i < 2; ++i)
        {
            m_handles->luminanceAdaptTextures[i] = bgfx::createTexture2D(
                1,
                1,
                false,
                1,
                bgfx::TextureFormat::RGBA16F,
                rtFlags
            );
            bgfx::Attachment attachment;
            attachment.init(m_handles->luminanceAdaptTextures[i], bgfx::Access::Write);
            m_handles->luminanceAdaptFrameBuffers[i] = bgfx::createFrameBuffer(1, &attachment, true);
        }
        bgfx::setViewRect(ViewIdLuminanceAdapt, 0, 0, 1, 1);
        bgfx::setViewClear(ViewIdLuminanceAdapt, BGFX_CLEAR_NONE, 0, 1.0f, 0);

        // 9. Configure skybox and scene views (Views 4 and 5) to draw on the HDR framebuffer
        bgfx::setViewFrameBuffer(ViewIdSkybox, m_handles->hdrFrameBuffer);
        bgfx::setViewRect(ViewIdSkybox, 0, 0, static_cast<std::uint16_t>(m_width), static_cast<std::uint16_t>(m_height));
        bgfx::setViewClear(ViewIdSkybox, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x101820ff, 1.0f, 0);

        bgfx::setViewFrameBuffer(ViewIdScene, m_handles->hdrFrameBuffer);
        bgfx::setViewRect(ViewIdScene, 0, 0, static_cast<std::uint16_t>(m_width), static_cast<std::uint16_t>(m_height));
        bgfx::setViewClear(ViewIdScene, BGFX_CLEAR_NONE, 0, 1.0f, 0);

        // 10. Configure final Postprocess view to draw on the backbuffer
        bgfx::setViewFrameBuffer(ViewIdPostProcess, BGFX_INVALID_HANDLE);
        bgfx::setViewRect(ViewIdPostProcess, 0, 0, static_cast<std::uint16_t>(m_width), static_cast<std::uint16_t>(m_height));
        bgfx::setViewClear(ViewIdPostProcess, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);
    }

    void Renderer::beginFrame()
    {
        m_stats = {};
        bgfx::dbgTextClear();
        for (bgfx::ViewId i = 0; i < ViewCount; ++i)
        {
            bgfx::touch(i);
        }
        m_sceneActive = false;
    }

    void Renderer::setFrameStats(float deltaSeconds)
    {
        m_deltaTime = deltaSeconds;
        m_frameTimeAccumulator += deltaSeconds;
        ++m_frameCounter;

        if(m_frameTimeAccumulator >= 0.25f)
        {
            m_fps = static_cast<float>(m_frameCounter) / m_frameTimeAccumulator;
            m_frameTimeAccumulator = 0.0f;
            m_frameCounter = 0;
        }
    }

    TextureHandle Renderer::loadTexture(const std::string& path, bool srgb)
    {
        if(path.empty())
        {
            return {};
        }

        const std::string cacheKey = path + (srgb ? "|srgb" : "|linear");
        const auto existing = m_handles->textureCache.find(cacheKey);
        if(existing != m_handles->textureCache.end())
        {
            return existing->second;
        }

        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
        if(pixels == nullptr)
        {
            throw std::runtime_error("Failed to load texture: " + path);
        }

        TextureHandle handle = createTextureFromRgbaPixels(pixels, static_cast<std::uint16_t>(width), static_cast<std::uint16_t>(height), srgb);
        stbi_image_free(pixels);

        m_handles->textureCache[cacheKey] = handle;
        return handle;
    }

    TextureHandle Renderer::loadTextureFromMemory(const std::vector<std::uint8_t>& encodedData, const std::string& debugName, bool srgb)
    {
        if(encodedData.empty())
        {
            return {};
        }

        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc* pixels = stbi_load_from_memory(encodedData.data(), static_cast<int>(encodedData.size()), &width, &height, &channels, 4);
        if(pixels == nullptr)
        {
            throw std::runtime_error("Failed to load embedded texture: " + debugName);
        }

        TextureHandle handle = createTextureFromRgbaPixels(pixels, static_cast<std::uint16_t>(width), static_cast<std::uint16_t>(height), srgb);
        stbi_image_free(pixels);
        return handle;
    }

    void Renderer::loadMaterialTextures(Material& material)
    {
        auto loadSlot = [this](TextureSlot& slot) {
            if(slot.handle.isValid())
            {
                return;
            }

            if(!slot.encodedData.empty())
            {
                slot.handle = loadTextureFromMemory(slot.encodedData, slot.debugName, slot.srgb);
            }
            else if(!slot.path.empty())
            {
                slot.handle = loadTexture(slot.path, slot.srgb);
            }
        };

        loadSlot(material.albedoTexture);
        loadSlot(material.normalTexture);
        loadSlot(material.metallicRoughnessTexture);
        loadSlot(material.aoTexture);
        loadSlot(material.emissiveTexture);
    }

    void Renderer::destroyTexture(TextureHandle handle)
    {
        if(!handle.isValid() || handle.id >= m_handles->gpuTextures.size())
        {
            return;
        }

        Handles::GpuTexture& texture = m_handles->gpuTextures[handle.id];
        if(!texture.alive)
        {
            return;
        }

        if(bgfx::isValid(texture.texture))
        {
            bgfx::destroy(texture.texture);
            texture.texture = BGFX_INVALID_HANDLE;
        }

        texture.alive = false;
    }

    MeshHandle Renderer::createMesh(const MeshData& meshData)
    {
        if(meshData.vertices.empty() || meshData.indices.empty())
        {
            return {};
        }

        std::vector<PosNormalColorVertex> vertices;
        vertices.reserve(meshData.vertices.size());
        for(const MeshVertex& vertex : meshData.vertices)
        {
            vertices.push_back({
                vertex.position.x,
                vertex.position.y,
                vertex.position.z,
                vertex.normal.x,
                vertex.normal.y,
                vertex.normal.z,
                White,
                vertex.uv.x,
                vertex.uv.y,
                vertex.tangent.x,
                vertex.tangent.y,
                vertex.tangent.z,
                vertex.tangent.w
            });
        }

        Handles::GpuMesh gpuMesh{};
        gpuMesh.vertexBuffer = bgfx::createVertexBuffer(
            bgfx::copy(vertices.data(), static_cast<std::uint32_t>(vertices.size() * sizeof(PosNormalColorVertex))),
            PosNormalColorVertex::layout);
        gpuMesh.indexBuffer = bgfx::createIndexBuffer(
            bgfx::copy(meshData.indices.data(), static_cast<std::uint32_t>(meshData.indices.size() * sizeof(std::uint32_t))),
            BGFX_BUFFER_INDEX32);
        gpuMesh.alive = bgfx::isValid(gpuMesh.vertexBuffer) && bgfx::isValid(gpuMesh.indexBuffer);

        if(!gpuMesh.alive)
        {
            if(bgfx::isValid(gpuMesh.indexBuffer))
            {
                bgfx::destroy(gpuMesh.indexBuffer);
            }

            if(bgfx::isValid(gpuMesh.vertexBuffer))
            {
                bgfx::destroy(gpuMesh.vertexBuffer);
            }

            return {};
        }

        m_handles->gpuMeshes.push_back(gpuMesh);
        return { static_cast<std::uint32_t>(m_handles->gpuMeshes.size() - 1) };
    }

    void Renderer::destroyMesh(MeshHandle handle)
    {
        if(!handle.isValid() || handle.id >= m_handles->gpuMeshes.size())
        {
            return;
        }

        Handles::GpuMesh& gpuMesh = m_handles->gpuMeshes[handle.id];
        if(!gpuMesh.alive)
        {
            return;
        }

        if(bgfx::isValid(gpuMesh.indexBuffer))
        {
            bgfx::destroy(gpuMesh.indexBuffer);
            gpuMesh.indexBuffer = BGFX_INVALID_HANDLE;
        }

        if(bgfx::isValid(gpuMesh.vertexBuffer))
        {
            bgfx::destroy(gpuMesh.vertexBuffer);
            gpuMesh.vertexBuffer = BGFX_INVALID_HANDLE;
        }

        gpuMesh.alive = false;
    }

    void Renderer::beginScene(const Camera& camera)
    {
        m_activeCamera = camera;

        const Vec3 forward = camera.forward();
        const Vec3 target = camera.position + forward;
        const bx::Vec3 eye{ camera.position.x, camera.position.y, camera.position.z };
        const bx::Vec3 at{ target.x, target.y, target.z };
        const bx::Vec3 up{ 0.0f, 1.0f, 0.0f };

        float view[16];
        float projection[16];
        bx::mtxLookAt(view, eye, at, up, bx::Handedness::Right);
        bx::mtxProj(
            projection,
            camera.verticalFovDegrees,
            static_cast<float>(m_width) / static_cast<float>(m_height),
            camera.nearPlane,
            camera.farPlane,
            bgfx::getCaps()->homogeneousDepth,
            bx::Handedness::Right);

        // Copy previous viewProj matrix
        std::memcpy(m_prevViewProj, m_currentViewProj, sizeof(m_currentViewProj));

        // Compute current viewProj matrix (non-jittered)
        float currentViewProj[16];
        bx::mtxMul(currentViewProj, view, projection);
        std::memcpy(m_currentViewProj, currentViewProj, sizeof(m_currentViewProj));

        // If previous is zero, initialize it to current viewProj
        bool prevIsZero = true;
        for (int i = 0; i < 16; ++i)
        {
            if (m_prevViewProj[i] != 0.0f)
            {
                prevIsZero = false;
                break;
            }
        }
        if (prevIsZero)
        {
            std::memcpy(m_prevViewProj, m_currentViewProj, sizeof(m_currentViewProj));
        }

        // Upload current and previous ViewProj to the shader
        bgfx::setUniform(m_handles->currentViewProjUniform, m_currentViewProj);
        bgfx::setUniform(m_handles->prevViewProjUniform, m_prevViewProj);

        // Store previous jitter
        m_prevJitterX = m_jitterX;
        m_prevJitterY = m_jitterY;

        // Calculate Halton Jitter for TAA
        static constexpr float Halton23[8][2] = {
            {  0.0f,      -0.166667f },
            { -0.25f,      0.166667f },
            {  0.25f,     -0.388889f },
            { -0.375f,    -0.055556f },
            {  0.125f,     0.277778f },
            { -0.125f,    -0.277778f },
            {  0.375f,     0.055556f },
            { -0.4375f,    0.388889f }
        };

        if (m_taaEnabled && m_postProcessMode == 0)
        {
            m_jitterIndex = (m_jitterIndex + 1) % 8;
            float dx = Halton23[m_jitterIndex][0];
            float dy = Halton23[m_jitterIndex][1];
            m_jitterX = 2.0f * dx / static_cast<float>(m_width);
            m_jitterY = 2.0f * dy / static_cast<float>(m_height);
        }
        else
        {
            m_jitterX = 0.0f;
            m_jitterY = 0.0f;
        }

        float projectionJittered[16];
        std::memcpy(projectionJittered, projection, sizeof(projection));
        projectionJittered[8] += m_jitterX;
        projectionJittered[9] += m_jitterY;

        bgfx::setViewTransform(ViewIdSkybox, view, projectionJittered);
        bgfx::setViewTransform(ViewIdScene, view, projectionJittered);
        m_sceneActive = true;
    }

    void Renderer::setDirectionalLight(const DirectionalLight& light)
    {
        m_light = light;
        m_light.direction = normalize(m_light.direction);
        m_light.intensity = std::max(m_light.intensity, 0.0f);

        // Now calculate Cascaded Shadow Map (CSM) matrices
        float aspect = static_cast<float>(m_width) / static_cast<float>(m_height);
        float fovRad = m_activeCamera.verticalFovDegrees * (bx::kPi / 180.0f);
        float tanHalfFovy = std::tan(fovRad * 0.5f);
        float tanHalfFovx = tanHalfFovy * aspect;

        float cascadeSplits[4] = { 10.0f, 30.0f, 75.0f, 160.0f };

        // We will store the final shadow matrices (with bias) to send to the shader
        float shadowMatrices[4 * 16];

        // Setup bias matrix
        const float sy = bgfx::getCaps()->originBottomLeft ? 0.5f : -0.5f;
        const float sz = bgfx::getCaps()->homogeneousDepth ? 0.5f : 1.0f;
        const float tz = bgfx::getCaps()->homogeneousDepth ? 0.5f : 0.0f;

        float biasMtx[16] = {
            0.5f, 0.0f, 0.0f, 0.0f,
            0.0f, sy,   0.0f, 0.0f,
            0.0f, 0.0f, sz,   0.0f,
            0.5f, 0.5f, tz,   1.0f,
        };

        Vec3 camForward = m_activeCamera.forward();

        for (int i = 0; i < 4; ++i)
        {
            float zNear = (i == 0) ? m_activeCamera.nearPlane : cascadeSplits[i - 1];
            float zFar = cascadeSplits[i];

            // Centroid of the split frustum along Z axis
            float zCenter = (zNear + zFar) * 0.5f;

            // Far plane width and height
            float xFar = zFar * tanHalfFovx;
            float yFar = zFar * tanHalfFovy;

            // Radius of bounding sphere (from center to far corner)
            float dz = zFar - zCenter;
            float radius = std::sqrt(xFar * xFar + yFar * yFar + dz * dz);
            radius *= 1.05f; // small padding to avoid clipping at boundaries

            // Centroid in world space
            Vec3 centerWorld = m_activeCamera.position + camForward * zCenter;

            // Construct UP vector for the light view matrix
            Vec3 lightUp = { 0.0f, 1.0f, 0.0f };
            if (std::abs(m_light.direction.y) > 0.99f)
            {
                lightUp = { 0.0f, 0.0f, 1.0f };
            }

            // Calculate light axes for snapping
            Vec3 lightRight = normalize(cross(lightUp, m_light.direction));
            Vec3 lightUpAxis = cross(m_light.direction, lightRight);

            // Snap centerWorld to the shadow map texel grid to prevent flickering/shimmering
            float texelSizeWorld = (2.0f * radius) / 4096.0f;
            float x = dot(centerWorld, lightRight);
            float y = dot(centerWorld, lightUpAxis);
            float snappedX = std::floor(x / texelSizeWorld) * texelSizeWorld;
            float snappedY = std::floor(y / texelSizeWorld) * texelSizeWorld;
            float dx = snappedX - x;
            float dy = snappedY - y;
            Vec3 snappedCenterWorld = centerWorld + lightRight * dx + lightUpAxis * dy;

            // Positioning the light far enough back to catch shadow casters in front of the camera frustum
            Vec3 lightEye = snappedCenterWorld - m_light.direction * (radius * 4.0f);
            Vec3 lightTarget = snappedCenterWorld;

            float lightView[16];
            bx::mtxLookAt(lightView, 
                bx::Vec3{lightEye.x, lightEye.y, lightEye.z}, 
                bx::Vec3{lightTarget.x, lightTarget.y, lightTarget.z}, 
                bx::Vec3{lightUp.x, lightUp.y, lightUp.z}, 
                bx::Handedness::Right);

            float lightProj[16];
            bx::mtxOrtho(lightProj, 
                -radius, radius, 
                -radius, radius, 
                0.0f, radius * 8.0f, 
                0.0f, 
                bgfx::getCaps()->homogeneousDepth, 
                bx::Handedness::Right);

            // Set the view transform for this cascade
            bgfx::setViewTransform(static_cast<bgfx::ViewId>(i), lightView, lightProj);

            // Calculate the shadow matrix for this cascade: shadowMtx = LightView * LightProj * Bias
            float mtxVP[16];
            bx::mtxMul(mtxVP, lightView, lightProj);
            
            float shadowMtx[16];
            bx::mtxMul(shadowMtx, mtxVP, biasMtx);

            // Store in the float array
            std::memcpy(&shadowMatrices[i * 16], shadowMtx, sizeof(shadowMtx));
        }

        std::memcpy(m_shadowMatrices, shadowMatrices, sizeof(shadowMatrices));
        m_cascadeSplits[0] = cascadeSplits[0];
        m_cascadeSplits[1] = cascadeSplits[1];
        m_cascadeSplits[2] = cascadeSplits[2];
        m_cascadeSplits[3] = cascadeSplits[3];
    }

    void Renderer::drawSkybox(const Skybox& skybox)
    {
        if(!m_sceneActive)
        {
            return;
        }

        const Transform transform{
            m_activeCamera.position,
            {},
            { m_activeCamera.farPlane * 0.4f, m_activeCamera.farPlane * 0.4f, m_activeCamera.farPlane * 0.4f }
        };

        setObjectTransform(transform);

        const float horizon[] = { skybox.horizon.r, skybox.horizon.g, skybox.horizon.b, skybox.horizon.a };
        const float zenith[] = { skybox.zenith.r, skybox.zenith.g, skybox.zenith.b, skybox.zenith.a };
        const float glow[] = { skybox.sunGlow.r, skybox.sunGlow.g, skybox.sunGlow.b, skybox.sunGlow.a };
        const float lightDirection[] = { m_light.direction.x, m_light.direction.y, m_light.direction.z, 0.0f };

        bgfx::setUniform(m_handles->skyHorizon, horizon);
        bgfx::setUniform(m_handles->skyZenith, zenith);
        bgfx::setUniform(m_handles->skySunGlow, glow);
        bgfx::setUniform(m_handles->lightDirection, lightDirection);
        bgfx::setVertexBuffer(0, m_handles->cubeVertexBuffer);
        bgfx::setIndexBuffer(m_handles->cubeIndexBuffer);
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_ALWAYS);
        bgfx::submit(ViewIdSkybox, m_handles->skyProgram);
        ++m_stats.drawCalls;
     }
 
     void Renderer::drawGround(float size, const Material& material)
     {
         if(!m_sceneActive)
         {
             return;
         }
 
         const Transform transform{ { 0.0f, 0.0f, 0.0f }, {}, { size, 1.0f, size } };
         setObjectTransform(transform);
         setPrevObjectTransform(transform);
 
         setPbrUniforms(material);
         bindShadowUniformsAndTextures();
         bgfx::setVertexBuffer(0, m_handles->planeVertexBuffer);
         bgfx::setIndexBuffer(m_handles->planeIndexBuffer);
         bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);
         bgfx::submit(ViewIdScene, m_handles->meshProgram);
         ++m_stats.drawCalls;
         ++m_stats.meshDraws;
     }

    void Renderer::drawCube(const Transform& transform, const Transform& prevTransform, const Material& material)
    {
        if(!m_sceneActive)
        {
            return;
        }

        // 1. Submit to the 4 shadow views
        for (bgfx::ViewId i = 0; i < 4; ++i)
        {
            setObjectTransform(transform);
            bgfx::setVertexBuffer(0, m_handles->cubeVertexBuffer);
            bgfx::setIndexBuffer(m_handles->cubeIndexBuffer);
            bgfx::setState(BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW);
            bgfx::submit(i, m_handles->shadowProgram);
        }

        // 2. Submit to the main view
        setObjectTransform(transform);
        setPrevObjectTransform(prevTransform);
        setPbrUniforms(material);
        bindShadowUniformsAndTextures();
        bgfx::setVertexBuffer(0, m_handles->cubeVertexBuffer);
        bgfx::setIndexBuffer(m_handles->cubeIndexBuffer);
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);
        bgfx::submit(ViewIdScene, m_handles->meshProgram);

        m_stats.drawCalls += 5;
        m_stats.meshDraws++;
    }

    void Renderer::drawSphere(const Transform& transform, const Transform& prevTransform, const Material& material)
    {
        if(!m_sceneActive)
        {
            return;
        }

        // 1. Submit to the 4 shadow views
        for (bgfx::ViewId i = 0; i < 4; ++i)
        {
            setObjectTransform(transform);
            bgfx::setVertexBuffer(0, m_handles->sphereVertexBuffer);
            bgfx::setIndexBuffer(m_handles->sphereIndexBuffer);
            bgfx::setState(BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW);
            bgfx::submit(i, m_handles->shadowProgram);
        }

        // 2. Submit to the main view
        setObjectTransform(transform);
        setPrevObjectTransform(prevTransform);
        setPbrUniforms(material);
        bindShadowUniformsAndTextures();
        bgfx::setVertexBuffer(0, m_handles->sphereVertexBuffer);
        bgfx::setIndexBuffer(m_handles->sphereIndexBuffer);
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);
        bgfx::submit(ViewIdScene, m_handles->meshProgram);

        m_stats.drawCalls += 5;
        m_stats.meshDraws++;
    }

    void Renderer::drawMesh(MeshHandle mesh, const Transform& transform, const Transform& prevTransform, const Material& material)
    {
        if(!m_sceneActive || !mesh.isValid() || mesh.id >= m_handles->gpuMeshes.size())
        {
            return;
        }

        const Handles::GpuMesh& gpuMesh = m_handles->gpuMeshes[mesh.id];
        if(!gpuMesh.alive)
        {
            return;
        }

        // 1. Submit to the 4 shadow views
        for (bgfx::ViewId i = 0; i < 4; ++i)
        {
            setObjectTransform(transform);
            bgfx::setVertexBuffer(0, gpuMesh.vertexBuffer);
            bgfx::setIndexBuffer(gpuMesh.indexBuffer);
            bgfx::setState(BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW);
            bgfx::submit(i, m_handles->shadowProgram);
        }

        // 2. Submit to the main view
        setObjectTransform(transform);
        setPrevObjectTransform(prevTransform);
        setPbrUniforms(material);
        bindShadowUniformsAndTextures();
        bgfx::setVertexBuffer(0, gpuMesh.vertexBuffer);
        bgfx::setIndexBuffer(gpuMesh.indexBuffer);
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);
        bgfx::submit(ViewIdScene, m_handles->meshProgram);

        m_stats.drawCalls += 5;
        m_stats.meshDraws++;
    }

    void Renderer::drawSun(const DirectionalLight& light, float distance, float size)
    {
        const Vec3 sunPosition = m_activeCamera.position + normalize(light.direction) * -distance;
        const Transform transform{ sunPosition, {}, { size, size, size } };

        setObjectTransform(transform);
        setPrevObjectTransform(transform);

        setPbrUniforms({ light.color, 0.0f, 0.18f, 7.5f });
        bindShadowUniformsAndTextures();
        bgfx::setVertexBuffer(0, m_handles->sphereVertexBuffer);
        bgfx::setIndexBuffer(m_handles->sphereIndexBuffer);
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);
        bgfx::submit(ViewIdScene, m_handles->meshProgram);
        ++m_stats.drawCalls;
        ++m_stats.meshDraws;
    }

    void Renderer::bindShadowUniformsAndTextures()
    {
        // Bind shadow map texture to slot 5 with explicit sampler flags
        bgfx::setTexture(5, m_handles->shadowMapSampler, m_handles->shadowTexture, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_COMPARE_LEQUAL);

        // Upload shadow matrices
        bgfx::setUniform(m_handles->shadowMtxUniform, m_shadowMatrices, 4);

        // Upload camera forward vector
        Vec3 forward = m_activeCamera.forward();
        float camForward[] = { forward.x, forward.y, forward.z, 0.0f };
        bgfx::setUniform(m_handles->cameraForwardUniform, camForward);

        // Upload cascade splits
        float splits[] = { m_cascadeSplits[0], m_cascadeSplits[1], m_cascadeSplits[2], m_cascadeSplits[3] };
        bgfx::setUniform(m_handles->cascadeSplitsUniform, splits);
    }

    void Renderer::endFrame()
    {
        // 1. Luminance Downsampling Chain (Pass 0: Log-Luminance; Passes 1-4: box averages)
        {
            // Pass 0: Downsample HDR scene texture to luminanceDownTextures[0] (256x256)
            float downParams[4] = { 0.0f, 1.0f / 256.0f, 0.0f, 0.0f }; // passIndex = 0 (log-lum)
            bgfx::setUniform(m_handles->downParamsUniform, downParams);
            bgfx::setTexture(0, m_handles->textureSampler, m_handles->hdrColorTexture);
            renderFullScreenQuad(ViewIdLuminanceDown0, m_handles->luminanceDownProgram);

            // Passes 1-4: Progressively downsample down to 1x1
            std::uint16_t size = 256;
            for (int i = 1; i < 5; ++i)
            {
                size /= 4; // 64, 16, 4, 1
                bgfx::ViewId viewId = static_cast<bgfx::ViewId>(ViewIdLuminanceDown0 + i);
                float downParamsBox[4] = { 1.0f, 1.0f / static_cast<float>(size * 4), 0.0f, 0.0f }; // passIndex = 1 (box average)
                bgfx::setUniform(m_handles->downParamsUniform, downParamsBox);
                bgfx::setTexture(0, m_handles->textureSampler, m_handles->luminanceDownTextures[i - 1]);
                renderFullScreenQuad(viewId, m_handles->luminanceDownProgram);
            }
        }

        // 2. Eye Adaptation Pass (blends current 1x1 luminance with previous adapted linear luminance)
        {
            bgfx::setViewFrameBuffer(ViewIdLuminanceAdapt, m_handles->luminanceAdaptFrameBuffers[m_luminanceIndex]);

            float adaptParams[4] = { m_deltaTime, 1.5f, 1.2f, 0.0f }; // dt, speedUp, speedDown, unused
            bgfx::setUniform(m_handles->adaptationParamsUniform, adaptParams);
            bgfx::setTexture(0, m_handles->currentTexSampler, m_handles->luminanceDownTextures[4]);
            bgfx::setTexture(1, m_handles->historyTexSampler, m_handles->luminanceAdaptTextures[1 - m_luminanceIndex]);
            renderFullScreenQuad(ViewIdLuminanceAdapt, m_handles->luminanceAdaptProgram);
        }

        // 3. Temporal Anti-Aliasing (TAA) Resolve Pass
        {
            bgfx::setViewFrameBuffer(ViewIdTAA, m_handles->taaHistoryFrameBuffers[m_taaHistoryIndex]);

            float feedback = m_taaEnabled ? 0.95f : 0.0f;
            float taaParams[4] = { feedback, 1.0f / static_cast<float>(m_width), 1.0f / static_cast<float>(m_height), 0.0f };
            bgfx::setUniform(m_handles->taaParamsUniform, taaParams);

            float taaJitter[4] = { m_jitterX * 0.5f, -m_jitterY * 0.5f, m_prevJitterX * 0.5f, -m_prevJitterY * 0.5f };
            bgfx::setUniform(m_handles->taaJitterUniform, taaJitter);

            bgfx::setTexture(0, m_handles->currentTexSampler, m_handles->hdrColorTexture);
            bgfx::setTexture(1, m_handles->historyTexSampler, m_handles->taaHistoryTextures[1 - m_taaHistoryIndex]);
            bgfx::setTexture(2, m_handles->velocityTexSampler, m_handles->velocityTexture);
            renderFullScreenQuad(ViewIdTAA, m_handles->taaProgram);
        }

        // 4. Depth of Field (DoF) Pass (using TAA resolved texture as source)
        {
            float dofParams[4];
            if (m_dofEnabled)
            {
                dofParams[0] = 8.5f;   // focusDistance
                dofParams[1] = 5.0f;   // focusRange
                dofParams[2] = m_activeCamera.nearPlane;
                dofParams[3] = m_activeCamera.farPlane;
            }
            else
            {
                dofParams[0] = 0.0f;
                dofParams[1] = 999999.0f;
                dofParams[2] = m_activeCamera.nearPlane;
                dofParams[3] = m_activeCamera.farPlane;
            }
            bgfx::setUniform(m_handles->dofParamsUniform, dofParams);
            bgfx::setTexture(0, m_handles->currentTexSampler, m_handles->taaHistoryTextures[m_taaHistoryIndex]);
            bgfx::setTexture(1, m_handles->depthTexSampler, m_handles->hdrDepthTexture);
            renderFullScreenQuad(ViewIdDepthOfField, m_handles->dofProgram);
        }

        // 5. Motion Blur Pass (using DoF output as source)
        {
            float mbParams[4] = {
                m_motionBlurEnabled ? 1.5f : 0.0f, // blurScale
                0.05f,                             // maxVelocityClamp
                0.0f,
                0.0f
            };
            bgfx::setUniform(m_handles->motionBlurParamsUniform, mbParams);
            bgfx::setTexture(0, m_handles->currentTexSampler, m_handles->dofTexture);
            bgfx::setTexture(1, m_handles->velocityTexSampler, m_handles->velocityTexture);
            renderFullScreenQuad(ViewIdMotionBlur, m_handles->motionBlurProgram);
        }

        // 6. Bloom Downsample passes (working on Motion Blur output)
        // Pass 0: Downsample motion-blurred scene to bloomDownTextures[0] and apply highlight prefilter
        {
            float downParams[4] = { m_bloomThreshold, 1.0f / static_cast<float>(m_width), 1.0f / static_cast<float>(m_height), 0.0f };
            bgfx::setUniform(m_handles->downParamsUniform, downParams);
            bgfx::setTexture(0, m_handles->textureSampler, m_handles->motionBlurTexture);
            renderFullScreenQuad(ViewIdBloomDown0, m_handles->bloomDownProgram);
        }

        // Pass 1 to 3: Successively downsample bloom mips
        std::uint16_t currentW = static_cast<std::uint16_t>(m_width / 2);
        std::uint16_t currentH = static_cast<std::uint16_t>(m_height / 2);
        for (int i = 1; i < 4; ++i)
        {
            bgfx::ViewId viewId = static_cast<bgfx::ViewId>(ViewIdBloomDown0 + i);
            float downParams[4] = { 1.0f, 1.0f / static_cast<float>(currentW), 1.0f / static_cast<float>(currentH), 1.0f }; // passIndex = 1 (no threshold)
            bgfx::setUniform(m_handles->downParamsUniform, downParams);
            bgfx::setTexture(0, m_handles->textureSampler, m_handles->bloomDownTextures[i - 1]);
            renderFullScreenQuad(viewId, m_handles->bloomDownProgram);

            currentW = std::max<std::uint16_t>(1, currentW / 2);
            currentH = std::max<std::uint16_t>(1, currentH / 2);
        }

        // 7. Bloom Upsample passes
        std::uint16_t upSizes[3][2];
        upSizes[0][0] = std::max<std::uint16_t>(1, static_cast<std::uint16_t>(m_width) / 8);
        upSizes[0][1] = std::max<std::uint16_t>(1, static_cast<std::uint16_t>(m_height) / 8);
        upSizes[1][0] = std::max<std::uint16_t>(1, static_cast<std::uint16_t>(m_width) / 4);
        upSizes[1][1] = std::max<std::uint16_t>(1, static_cast<std::uint16_t>(m_height) / 4);
        upSizes[2][0] = std::max<std::uint16_t>(1, static_cast<std::uint16_t>(m_width) / 2);
        upSizes[2][1] = std::max<std::uint16_t>(1, static_cast<std::uint16_t>(m_height) / 2);

        // Pass 0: Upsample from bloomDownTextures[3] (W/16) + bloomDownTextures[2] (W/8) to bloomUpTextures[0] (W/8)
        {
            float upParams[4] = { 0.85f, 1.0f / static_cast<float>(upSizes[0][0] / 2), 1.0f / static_cast<float>(upSizes[0][1] / 2), 0.85f };
            bgfx::setUniform(m_handles->upParamsUniform, upParams);
            bgfx::setTexture(0, m_handles->lowTexSampler, m_handles->bloomDownTextures[3]);
            bgfx::setTexture(1, m_handles->highTexSampler, m_handles->bloomDownTextures[2]);
            renderFullScreenQuad(ViewIdBloomUp0, m_handles->bloomUpProgram);
        }

        // Pass 1: Upsample from bloomUpTextures[0] (W/8) + bloomDownTextures[1] (W/4) to bloomUpTextures[1] (W/4)
        {
            float upParams[4] = { 0.85f, 1.0f / static_cast<float>(upSizes[1][0] / 2), 1.0f / static_cast<float>(upSizes[1][1] / 2), 0.85f };
            bgfx::setUniform(m_handles->upParamsUniform, upParams);
            bgfx::setTexture(0, m_handles->lowTexSampler, m_handles->bloomUpTextures[0]);
            bgfx::setTexture(1, m_handles->highTexSampler, m_handles->bloomDownTextures[1]);
            renderFullScreenQuad(ViewIdBloomUp1, m_handles->bloomUpProgram);
        }

        // Pass 2: Upsample from bloomUpTextures[1] (W/4) + bloomDownTextures[0] (W/2) to bloomUpTextures[2] (W/2)
        {
            float upParams[4] = { 0.85f, 1.0f / static_cast<float>(upSizes[2][0] / 2), 1.0f / static_cast<float>(upSizes[2][1] / 2), 0.85f };
            bgfx::setUniform(m_handles->upParamsUniform, upParams);
            bgfx::setTexture(0, m_handles->lowTexSampler, m_handles->bloomUpTextures[1]);
            bgfx::setTexture(1, m_handles->highTexSampler, m_handles->bloomDownTextures[0]);
            renderFullScreenQuad(ViewIdBloomUp2, m_handles->bloomUpProgram);
        }

        // 8. Final Postprocess pass (combines motion blurred scene and bloomUpTextures[2] to backbuffer)
        {
            float autoExposureVal = m_autoExposureEnabled ? 1.0f : 0.0f;
            float tonemapVal = m_tonemapEnabled ? 2.0f : 0.0f;
            float bloomIntensityVal = m_bloomEnabled ? m_bloomIntensity : 0.0f;
            float postParams[4] = {
                m_exposure,
                bloomIntensityVal,
                static_cast<float>(m_postProcessMode),
                autoExposureVal + tonemapVal
            };
            bgfx::setUniform(m_handles->postParamsUniform, postParams);
            bgfx::setTexture(0, m_handles->sceneTexSampler, m_handles->motionBlurTexture);
            bgfx::setTexture(1, m_handles->bloomTexSampler, m_handles->bloomUpTextures[2]);
            bgfx::setTexture(2, m_handles->luminanceTexSampler, m_handles->luminanceAdaptTextures[m_luminanceIndex]);
            renderFullScreenQuad(ViewIdPostProcess, m_handles->postProgram);
        }

        // Output debug text on the final post-process view
        bgfx::dbgTextPrintf(1, 1, 0x4f, "Arc Engine");
        bgfx::dbgTextPrintf(1, 2, 0x0f, "Renderer: %s", bgfx::getRendererName(bgfx::getRendererType()));
        bgfx::dbgTextPrintf(1, 3, 0x0f, "FPS: %.1f", m_fps);
        bgfx::dbgTextPrintf(1, 4, 0x0f, "Draw calls: %u | Meshes: %u | Shadows: Active", m_stats.drawCalls, m_stats.meshDraws);

        bgfx::dbgTextPrintf(1, 5, 0x0e, "TAA: %s | DoF: %s | Motion Blur: %s | Auto Exposure: %s | Bloom: %s",
            m_taaEnabled ? "ON" : "OFF",
            m_dofEnabled ? "ON" : "OFF",
            m_motionBlurEnabled ? "ON" : "OFF",
            m_autoExposureEnabled ? "ON" : "OFF",
            m_bloomEnabled ? "ON" : "OFF");
        bgfx::dbgTextPrintf(1, 6, 0x0e, "Tonemapping (ACES): %s", m_tonemapEnabled ? "ON" : "OFF");
        bgfx::dbgTextPrintf(1, 7, 0x0f, "Controls: Move: WASD/QE | Look: hold RMB | Fast: Shift | Quit: Esc");

        // Swap TAA and Luminance history buffers
        m_taaHistoryIndex = (m_taaHistoryIndex + 1) % 2;
        m_luminanceIndex = (m_luminanceIndex + 1) % 2;

        bgfx::frame();
    }

    std::uint32_t Renderer::width() const
    {
        return m_width;
    }

    std::uint32_t Renderer::height() const
    {
        return m_height;
    }

    const RenderStats& Renderer::stats() const
    {
        return m_stats;
    }

    void Renderer::createGeometry()
    {
        m_handles->cubeVertexBuffer = bgfx::createVertexBuffer(bgfx::makeRef(CubeVertices, sizeof(CubeVertices)), PosNormalColorVertex::layout);
        m_handles->cubeIndexBuffer = bgfx::createIndexBuffer(bgfx::makeRef(CubeIndices, sizeof(CubeIndices)));
        m_handles->planeVertexBuffer = bgfx::createVertexBuffer(bgfx::makeRef(PlaneVertices, sizeof(PlaneVertices)), PosNormalColorVertex::layout);
        m_handles->planeIndexBuffer = bgfx::createIndexBuffer(bgfx::makeRef(PlaneIndices, sizeof(PlaneIndices)));

        std::vector<PosNormalColorVertex> sphereVertices;
        std::vector<std::uint16_t> sphereIndices;
        buildSphere(sphereVertices, sphereIndices);
        m_handles->sphereVertexBuffer = bgfx::createVertexBuffer(
            bgfx::copy(sphereVertices.data(), static_cast<std::uint32_t>(sphereVertices.size() * sizeof(PosNormalColorVertex))),
            PosNormalColorVertex::layout);
        m_handles->sphereIndexBuffer = bgfx::createIndexBuffer(
            bgfx::copy(sphereIndices.data(), static_cast<std::uint32_t>(sphereIndices.size() * sizeof(std::uint16_t))));

        std::vector<PosNormalColorVertex> diskVertices;
        std::vector<std::uint16_t> diskIndices;
        buildDisk(diskVertices, diskIndices);
        m_handles->diskVertexBuffer = bgfx::createVertexBuffer(
            bgfx::copy(diskVertices.data(), static_cast<std::uint32_t>(diskVertices.size() * sizeof(PosNormalColorVertex))),
            PosNormalColorVertex::layout);
        m_handles->diskIndexBuffer = bgfx::createIndexBuffer(
            bgfx::copy(diskIndices.data(), static_cast<std::uint32_t>(diskIndices.size() * sizeof(std::uint16_t))));
    }

    TextureHandle Renderer::createTextureFromRgbaPixels(const void* pixels, std::uint16_t width, std::uint16_t height, bool srgb)
    {
        if(pixels == nullptr || width == 0 || height == 0)
        {
            return {};
        }

        std::uint64_t flags = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
        if(srgb)
        {
            flags |= BGFX_TEXTURE_SRGB;
        }

        Handles::GpuTexture texture{};
        texture.texture = bgfx::createTexture2D(
            width,
            height,
            false,
            1,
            bgfx::TextureFormat::RGBA8,
            flags,
            bgfx::copy(pixels, static_cast<std::uint32_t>(width) * static_cast<std::uint32_t>(height) * 4u));
        texture.alive = bgfx::isValid(texture.texture);

        if(!texture.alive)
        {
            return {};
        }

        m_handles->gpuTextures.push_back(texture);
        return { static_cast<std::uint32_t>(m_handles->gpuTextures.size() - 1) };
    }

    void Renderer::createDefaultTextures()
    {
        const std::uint32_t white = 0xffffffffu;
        const std::uint32_t black = 0xff000000u;
        const std::uint32_t flatNormal = 0xffff8080u;

        m_handles->defaultWhiteTexture = createTextureFromRgbaPixels(&white, 1, 1, false);
        m_handles->defaultBlackTexture = createTextureFromRgbaPixels(&black, 1, 1, false);
        m_handles->defaultNormalTexture = createTextureFromRgbaPixels(&flatNormal, 1, 1, false);
    }

    void Renderer::createPrograms()
    {
        m_handles->meshProgram = loadProgram(shaderPath("vs_mesh.sc.bin"), shaderPath("fs_mesh.sc.bin"));
        m_handles->skyProgram = loadProgram(shaderPath("vs_sky.sc.bin"), shaderPath("fs_sky.sc.bin"));
        m_handles->shadowProgram = loadProgram(shaderPath("vs_shadow.sc.bin"), shaderPath("fs_shadow.sc.bin"));
        m_handles->bloomDownProgram = loadProgram(shaderPath("vs_post.sc.bin"), shaderPath("fs_bloom_down.sc.bin"));
        m_handles->bloomUpProgram = loadProgram(shaderPath("vs_post.sc.bin"), shaderPath("fs_bloom_up.sc.bin"));
        m_handles->postProgram = loadProgram(shaderPath("vs_post.sc.bin"), shaderPath("fs_post.sc.bin"));
        m_handles->taaProgram = loadProgram(shaderPath("vs_post.sc.bin"), shaderPath("fs_taa.sc.bin"));
        m_handles->dofProgram = loadProgram(shaderPath("vs_post.sc.bin"), shaderPath("fs_dof.sc.bin"));
        m_handles->motionBlurProgram = loadProgram(shaderPath("vs_post.sc.bin"), shaderPath("fs_motion_blur.sc.bin"));
        m_handles->luminanceDownProgram = loadProgram(shaderPath("vs_post.sc.bin"), shaderPath("fs_luminance_down.sc.bin"));
        m_handles->luminanceAdaptProgram = loadProgram(shaderPath("vs_post.sc.bin"), shaderPath("fs_luminance_adapt.sc.bin"));

        if(!valid(m_handles->meshProgram) || !valid(m_handles->skyProgram) || 
           !valid(m_handles->shadowProgram) || !valid(m_handles->bloomDownProgram) || 
           !valid(m_handles->bloomUpProgram) || !valid(m_handles->postProgram) ||
           !valid(m_handles->taaProgram) || !valid(m_handles->dofProgram) ||
           !valid(m_handles->motionBlurProgram) || !valid(m_handles->luminanceDownProgram) ||
           !valid(m_handles->luminanceAdaptProgram))
        {
            throw std::runtime_error("Failed to create BGFX shader programs.");
        }

        m_handles->lightDirection = bgfx::createUniform("u_lightDirection", bgfx::UniformType::Vec4);
        m_handles->lightColor = bgfx::createUniform("u_lightColor", bgfx::UniformType::Vec4);
        m_handles->cameraData = bgfx::createUniform("u_cameraData", bgfx::UniformType::Vec4);
        m_handles->ambientColor = bgfx::createUniform("u_ambientColor", bgfx::UniformType::Vec4);
        m_handles->tint = bgfx::createUniform("u_tint", bgfx::UniformType::Vec4);
        m_handles->material = bgfx::createUniform("u_material", bgfx::UniformType::Vec4);
        m_handles->albedoSampler = bgfx::createUniform("s_albedo", bgfx::UniformType::Sampler);
        m_handles->normalSampler = bgfx::createUniform("s_normal", bgfx::UniformType::Sampler);
        m_handles->metallicRoughnessSampler = bgfx::createUniform("s_metallicRoughness", bgfx::UniformType::Sampler);
        m_handles->aoSampler = bgfx::createUniform("s_ao", bgfx::UniformType::Sampler);
        m_handles->emissiveSampler = bgfx::createUniform("s_emissive", bgfx::UniformType::Sampler);
        m_handles->skyHorizon = bgfx::createUniform("u_skyHorizon", bgfx::UniformType::Vec4);
        m_handles->skyZenith = bgfx::createUniform("u_skyZenith", bgfx::UniformType::Vec4);
        m_handles->skySunGlow = bgfx::createUniform("u_skySunGlow", bgfx::UniformType::Vec4);
        m_handles->shadowMtxUniform = bgfx::createUniform("u_shadowMtx", bgfx::UniformType::Mat4, 4);
        m_handles->shadowMapSampler = bgfx::createUniform("s_shadowMap", bgfx::UniformType::Sampler);
        m_handles->cameraForwardUniform = bgfx::createUniform("u_cameraForward", bgfx::UniformType::Vec4);
        m_handles->cascadeSplitsUniform = bgfx::createUniform("u_cascadeSplits", bgfx::UniformType::Vec4);
        m_handles->downParamsUniform = bgfx::createUniform("u_downParams", bgfx::UniformType::Vec4);
        m_handles->upParamsUniform = bgfx::createUniform("u_upParams", bgfx::UniformType::Vec4);
        m_handles->postParamsUniform = bgfx::createUniform("u_postParams", bgfx::UniformType::Vec4);
        m_handles->textureSampler = bgfx::createUniform("s_texture", bgfx::UniformType::Sampler);
        m_handles->sceneTexSampler = bgfx::createUniform("s_sceneTex", bgfx::UniformType::Sampler);
        m_handles->bloomTexSampler = bgfx::createUniform("s_bloomTex", bgfx::UniformType::Sampler);
        m_handles->lowTexSampler = bgfx::createUniform("s_lowTex", bgfx::UniformType::Sampler);
        m_handles->highTexSampler = bgfx::createUniform("s_highTex", bgfx::UniformType::Sampler);

        m_handles->taaParamsUniform = bgfx::createUniform("u_taaParams", bgfx::UniformType::Vec4);
        m_handles->dofParamsUniform = bgfx::createUniform("u_dofParams", bgfx::UniformType::Vec4);
        m_handles->motionBlurParamsUniform = bgfx::createUniform("u_motionBlurParams", bgfx::UniformType::Vec4);
        m_handles->adaptationParamsUniform = bgfx::createUniform("u_adaptationParams", bgfx::UniformType::Vec4);

        m_handles->currentTexSampler = bgfx::createUniform("s_currentTex", bgfx::UniformType::Sampler);
        m_handles->historyTexSampler = bgfx::createUniform("s_historyTex", bgfx::UniformType::Sampler);
        m_handles->velocityTexSampler = bgfx::createUniform("s_velocityTex", bgfx::UniformType::Sampler);
        m_handles->depthTexSampler = bgfx::createUniform("s_depthTex", bgfx::UniformType::Sampler);
        m_handles->luminanceTexSampler = bgfx::createUniform("s_luminanceTex", bgfx::UniformType::Sampler);

        m_handles->prevModelUniform = bgfx::createUniform("u_prevModel", bgfx::UniformType::Mat4);
        m_handles->prevViewProjUniform = bgfx::createUniform("u_prevViewProj", bgfx::UniformType::Mat4);
        m_handles->currentViewProjUniform = bgfx::createUniform("u_currentViewProj", bgfx::UniformType::Mat4);
        m_handles->taaJitterUniform = bgfx::createUniform("u_taaJitter", bgfx::UniformType::Vec4);
    }

    void Renderer::destroyGeometry()
    {
        if(m_handles == nullptr)
        {
            return;
        }

        for(std::uint32_t index = 0; index < m_handles->gpuMeshes.size(); ++index)
        {
            destroyMesh({ index });
        }

        m_handles->gpuMeshes.clear();

        for(std::uint32_t index = 0; index < m_handles->gpuTextures.size(); ++index)
        {
            destroyTexture({ index });
        }

        m_handles->gpuTextures.clear();
        m_handles->textureCache.clear();
        m_handles->defaultWhiteTexture = {};
        m_handles->defaultBlackTexture = {};
        m_handles->defaultNormalTexture = {};

        if(bgfx::isValid(m_handles->diskIndexBuffer))
        {
            bgfx::destroy(m_handles->diskIndexBuffer);
            m_handles->diskIndexBuffer = BGFX_INVALID_HANDLE;
        }

        if(bgfx::isValid(m_handles->diskVertexBuffer))
        {
            bgfx::destroy(m_handles->diskVertexBuffer);
            m_handles->diskVertexBuffer = BGFX_INVALID_HANDLE;
        }

        if(bgfx::isValid(m_handles->sphereIndexBuffer))
        {
            bgfx::destroy(m_handles->sphereIndexBuffer);
            m_handles->sphereIndexBuffer = BGFX_INVALID_HANDLE;
        }

        if(bgfx::isValid(m_handles->sphereVertexBuffer))
        {
            bgfx::destroy(m_handles->sphereVertexBuffer);
            m_handles->sphereVertexBuffer = BGFX_INVALID_HANDLE;
        }

        if(bgfx::isValid(m_handles->planeIndexBuffer))
        {
            bgfx::destroy(m_handles->planeIndexBuffer);
            m_handles->planeIndexBuffer = BGFX_INVALID_HANDLE;
        }

        if(bgfx::isValid(m_handles->planeVertexBuffer))
        {
            bgfx::destroy(m_handles->planeVertexBuffer);
            m_handles->planeVertexBuffer = BGFX_INVALID_HANDLE;
        }

        if(bgfx::isValid(m_handles->cubeIndexBuffer))
        {
            bgfx::destroy(m_handles->cubeIndexBuffer);
            m_handles->cubeIndexBuffer = BGFX_INVALID_HANDLE;
        }

        if(bgfx::isValid(m_handles->cubeVertexBuffer))
        {
            bgfx::destroy(m_handles->cubeVertexBuffer);
            m_handles->cubeVertexBuffer = BGFX_INVALID_HANDLE;
        }
    }

    void Renderer::destroyPrograms()
    {
        if(m_handles == nullptr)
        {
            return;
        }

        if(valid(m_handles->skySunGlow))
        {
            bgfx::destroy(m_handles->skySunGlow);
            m_handles->skySunGlow = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->skyZenith))
        {
            bgfx::destroy(m_handles->skyZenith);
            m_handles->skyZenith = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->skyHorizon))
        {
            bgfx::destroy(m_handles->skyHorizon);
            m_handles->skyHorizon = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->emissiveSampler))
        {
            bgfx::destroy(m_handles->emissiveSampler);
            m_handles->emissiveSampler = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->aoSampler))
        {
            bgfx::destroy(m_handles->aoSampler);
            m_handles->aoSampler = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->metallicRoughnessSampler))
        {
            bgfx::destroy(m_handles->metallicRoughnessSampler);
            m_handles->metallicRoughnessSampler = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->normalSampler))
        {
            bgfx::destroy(m_handles->normalSampler);
            m_handles->normalSampler = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->albedoSampler))
        {
            bgfx::destroy(m_handles->albedoSampler);
            m_handles->albedoSampler = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->material))
        {
            bgfx::destroy(m_handles->material);
            m_handles->material = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->tint))
        {
            bgfx::destroy(m_handles->tint);
            m_handles->tint = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->ambientColor))
        {
            bgfx::destroy(m_handles->ambientColor);
            m_handles->ambientColor = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->cameraData))
        {
            bgfx::destroy(m_handles->cameraData);
            m_handles->cameraData = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->lightColor))
        {
            bgfx::destroy(m_handles->lightColor);
            m_handles->lightColor = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->lightDirection))
        {
            bgfx::destroy(m_handles->lightDirection);
            m_handles->lightDirection = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->skyProgram))
        {
            bgfx::destroy(m_handles->skyProgram);
            m_handles->skyProgram = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->meshProgram))
        {
            bgfx::destroy(m_handles->meshProgram);
            m_handles->meshProgram = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->shadowProgram))
        {
            bgfx::destroy(m_handles->shadowProgram);
            m_handles->shadowProgram = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->shadowMtxUniform))
        {
            bgfx::destroy(m_handles->shadowMtxUniform);
            m_handles->shadowMtxUniform = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->shadowMapSampler))
        {
            bgfx::destroy(m_handles->shadowMapSampler);
            m_handles->shadowMapSampler = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->cameraForwardUniform))
        {
            bgfx::destroy(m_handles->cameraForwardUniform);
            m_handles->cameraForwardUniform = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->cascadeSplitsUniform))
        {
            bgfx::destroy(m_handles->cascadeSplitsUniform);
            m_handles->cascadeSplitsUniform = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->bloomDownProgram))
        {
            bgfx::destroy(m_handles->bloomDownProgram);
            m_handles->bloomDownProgram = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->bloomUpProgram))
        {
            bgfx::destroy(m_handles->bloomUpProgram);
            m_handles->bloomUpProgram = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->postProgram))
        {
            bgfx::destroy(m_handles->postProgram);
            m_handles->postProgram = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->downParamsUniform))
        {
            bgfx::destroy(m_handles->downParamsUniform);
            m_handles->downParamsUniform = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->upParamsUniform))
        {
            bgfx::destroy(m_handles->upParamsUniform);
            m_handles->upParamsUniform = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->postParamsUniform))
        {
            bgfx::destroy(m_handles->postParamsUniform);
            m_handles->postParamsUniform = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->textureSampler))
        {
            bgfx::destroy(m_handles->textureSampler);
            m_handles->textureSampler = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->sceneTexSampler))
        {
            bgfx::destroy(m_handles->sceneTexSampler);
            m_handles->sceneTexSampler = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->bloomTexSampler))
        {
            bgfx::destroy(m_handles->bloomTexSampler);
            m_handles->bloomTexSampler = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->lowTexSampler))
        {
            bgfx::destroy(m_handles->lowTexSampler);
            m_handles->lowTexSampler = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->highTexSampler))
        {
            bgfx::destroy(m_handles->highTexSampler);
            m_handles->highTexSampler = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->taaProgram))
        {
            bgfx::destroy(m_handles->taaProgram);
            m_handles->taaProgram = BGFX_INVALID_HANDLE;
        }
        if(valid(m_handles->dofProgram))
        {
            bgfx::destroy(m_handles->dofProgram);
            m_handles->dofProgram = BGFX_INVALID_HANDLE;
        }
        if(valid(m_handles->motionBlurProgram))
        {
            bgfx::destroy(m_handles->motionBlurProgram);
            m_handles->motionBlurProgram = BGFX_INVALID_HANDLE;
        }
        if(valid(m_handles->luminanceDownProgram))
        {
            bgfx::destroy(m_handles->luminanceDownProgram);
            m_handles->luminanceDownProgram = BGFX_INVALID_HANDLE;
        }
        if(valid(m_handles->luminanceAdaptProgram))
        {
            bgfx::destroy(m_handles->luminanceAdaptProgram);
            m_handles->luminanceAdaptProgram = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->taaParamsUniform))
        {
            bgfx::destroy(m_handles->taaParamsUniform);
            m_handles->taaParamsUniform = BGFX_INVALID_HANDLE;
        }
        if(valid(m_handles->dofParamsUniform))
        {
            bgfx::destroy(m_handles->dofParamsUniform);
            m_handles->dofParamsUniform = BGFX_INVALID_HANDLE;
        }
        if(valid(m_handles->motionBlurParamsUniform))
        {
            bgfx::destroy(m_handles->motionBlurParamsUniform);
            m_handles->motionBlurParamsUniform = BGFX_INVALID_HANDLE;
        }
        if(valid(m_handles->adaptationParamsUniform))
        {
            bgfx::destroy(m_handles->adaptationParamsUniform);
            m_handles->adaptationParamsUniform = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->currentTexSampler))
        {
            bgfx::destroy(m_handles->currentTexSampler);
            m_handles->currentTexSampler = BGFX_INVALID_HANDLE;
        }
        if(valid(m_handles->historyTexSampler))
        {
            bgfx::destroy(m_handles->historyTexSampler);
            m_handles->historyTexSampler = BGFX_INVALID_HANDLE;
        }
        if(valid(m_handles->velocityTexSampler))
        {
            bgfx::destroy(m_handles->velocityTexSampler);
            m_handles->velocityTexSampler = BGFX_INVALID_HANDLE;
        }
        if(valid(m_handles->depthTexSampler))
        {
            bgfx::destroy(m_handles->depthTexSampler);
            m_handles->depthTexSampler = BGFX_INVALID_HANDLE;
        }
        if(valid(m_handles->luminanceTexSampler))
        {
            bgfx::destroy(m_handles->luminanceTexSampler);
            m_handles->luminanceTexSampler = BGFX_INVALID_HANDLE;
        }

        if(valid(m_handles->prevModelUniform))
        {
            bgfx::destroy(m_handles->prevModelUniform);
            m_handles->prevModelUniform = BGFX_INVALID_HANDLE;
        }
        if(valid(m_handles->prevViewProjUniform))
        {
            bgfx::destroy(m_handles->prevViewProjUniform);
            m_handles->prevViewProjUniform = BGFX_INVALID_HANDLE;
        }
        if(valid(m_handles->currentViewProjUniform))
        {
            bgfx::destroy(m_handles->currentViewProjUniform);
            m_handles->currentViewProjUniform = BGFX_INVALID_HANDLE;
        }
        if(valid(m_handles->taaJitterUniform))
        {
            bgfx::destroy(m_handles->taaJitterUniform);
            m_handles->taaJitterUniform = BGFX_INVALID_HANDLE;
        }
    }

    void Renderer::setObjectTransform(const Transform& transform)
    {
        float matrix[16];
        bx::mtxSRT(
            matrix,
            transform.scale.x,
            transform.scale.y,
            transform.scale.z,
            transform.rotation.x,
            transform.rotation.y,
            transform.rotation.z,
            transform.position.x,
            transform.position.y,
            transform.position.z);
        bgfx::setTransform(matrix);
    }

    void Renderer::setPrevObjectTransform(const Transform& transform)
    {
        float matrix[16];
        bx::mtxSRT(
            matrix,
            transform.scale.x,
            transform.scale.y,
            transform.scale.z,
            transform.rotation.x,
            transform.rotation.y,
            transform.rotation.z,
            transform.position.x,
            transform.position.y,
            transform.position.z);
        bgfx::setUniform(m_handles->prevModelUniform, matrix);
    }

    void Renderer::setPbrUniforms(const Material& material)
    {
        const float tint[] = { material.baseColor.r, material.baseColor.g, material.baseColor.b, material.baseColor.a };
        const float materialData[] = { material.metallic, material.roughness, material.emissive, 0.0f };
        const float lightDirection[] = { m_light.direction.x, m_light.direction.y, m_light.direction.z, 0.0f };
        const float lightColor[] = { m_light.color.r, m_light.color.g, m_light.color.b, m_light.intensity };
        const float cameraData[] = { m_activeCamera.position.x, m_activeCamera.position.y, m_activeCamera.position.z, bgfx::getCaps()->originBottomLeft ? 1.0f : -1.0f };
        const float ambientColor[] = { 0.34f, 0.39f, 0.46f, 0.38f };

        bgfx::setUniform(m_handles->tint, tint);
        bgfx::setUniform(m_handles->material, materialData);
        bgfx::setUniform(m_handles->lightDirection, lightDirection);
        bgfx::setUniform(m_handles->lightColor, lightColor);
        bgfx::setUniform(m_handles->cameraData, cameraData);
        bgfx::setUniform(m_handles->ambientColor, ambientColor);

        auto textureOrDefault = [this](TextureHandle handle, TextureHandle fallback) -> bgfx::TextureHandle {
            const TextureHandle selected = handle.isValid() ? handle : fallback;
            if(selected.isValid() && selected.id < m_handles->gpuTextures.size() && m_handles->gpuTextures[selected.id].alive)
            {
                return m_handles->gpuTextures[selected.id].texture;
            }

            bgfx::TextureHandle invalid = BGFX_INVALID_HANDLE;
            return invalid;
        };

        bgfx::setTexture(0, m_handles->albedoSampler, textureOrDefault(material.albedoTexture.handle, m_handles->defaultWhiteTexture));
        bgfx::setTexture(1, m_handles->normalSampler, textureOrDefault(material.normalTexture.handle, m_handles->defaultNormalTexture));
        bgfx::setTexture(2, m_handles->metallicRoughnessSampler, textureOrDefault(material.metallicRoughnessTexture.handle, m_handles->defaultWhiteTexture));
        bgfx::setTexture(3, m_handles->aoSampler, textureOrDefault(material.aoTexture.handle, m_handles->defaultWhiteTexture));
        bgfx::setTexture(4, m_handles->emissiveSampler, textureOrDefault(material.emissiveTexture.handle, m_handles->defaultWhiteTexture));
    }

    std::string Renderer::shaderPath(const char* shaderName) const
    {
        const bgfx::RendererType::Enum rendererType = bgfx::getRendererType();
        const char* profile = "spirv";

        if(rendererType == bgfx::RendererType::Direct3D11 || rendererType == bgfx::RendererType::Direct3D12)
        {
            profile = "dx11";
        }
        else if(rendererType == bgfx::RendererType::OpenGL)
        {
            profile = "glsl";
        }

        return m_shaderDirectory + "/" + profile + "/" + shaderName;
    }
}
