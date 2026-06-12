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

        // Configure skybox and scene views (Views 4 and 5) to draw on the backbuffer (full screen)
        bgfx::setViewFrameBuffer(ViewIdSkybox, BGFX_INVALID_HANDLE);
        bgfx::setViewRect(ViewIdSkybox, 0, 0, static_cast<std::uint16_t>(m_width), static_cast<std::uint16_t>(m_height));
        bgfx::setViewClear(ViewIdSkybox, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x101820ff, 1.0f, 0);

        bgfx::setViewFrameBuffer(ViewIdScene, BGFX_INVALID_HANDLE);
        bgfx::setViewRect(ViewIdScene, 0, 0, static_cast<std::uint16_t>(m_width), static_cast<std::uint16_t>(m_height));
        bgfx::setViewClear(ViewIdScene, BGFX_CLEAR_NONE, 0, 1.0f, 0);
    }

    void Renderer::beginFrame()
    {
        m_stats = {};
        bgfx::dbgTextClear();
        for (bgfx::ViewId i = 0; i < 6; ++i)
        {
            bgfx::touch(i);
        }
        m_sceneActive = false;
    }

    void Renderer::setFrameStats(float deltaSeconds)
    {
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

        bgfx::setViewTransform(ViewIdSkybox, view, projection);
        bgfx::setViewTransform(ViewIdScene, view, projection);
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
 
         setPbrUniforms(material);
         bindShadowUniformsAndTextures();
         bgfx::setVertexBuffer(0, m_handles->planeVertexBuffer);
         bgfx::setIndexBuffer(m_handles->planeIndexBuffer);
         bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);
         bgfx::submit(ViewIdScene, m_handles->meshProgram);
         ++m_stats.drawCalls;
         ++m_stats.meshDraws;
     }



    void Renderer::drawCube(const Transform& transform, const Material& material)
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
        setPbrUniforms(material);
        bindShadowUniformsAndTextures();
        bgfx::setVertexBuffer(0, m_handles->cubeVertexBuffer);
        bgfx::setIndexBuffer(m_handles->cubeIndexBuffer);
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);
        bgfx::submit(ViewIdScene, m_handles->meshProgram);

        m_stats.drawCalls += 5;
        m_stats.meshDraws++;
    }

    void Renderer::drawSphere(const Transform& transform, const Material& material)
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
        setPbrUniforms(material);
        bindShadowUniformsAndTextures();
        bgfx::setVertexBuffer(0, m_handles->sphereVertexBuffer);
        bgfx::setIndexBuffer(m_handles->sphereIndexBuffer);
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);
        bgfx::submit(ViewIdScene, m_handles->meshProgram);

        m_stats.drawCalls += 5;
        m_stats.meshDraws++;
    }

    void Renderer::drawMesh(MeshHandle mesh, const Transform& transform, const Material& material)
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
        bgfx::dbgTextPrintf(1, 1, 0x4f, "Arc Engine");
        bgfx::dbgTextPrintf(1, 2, 0x0f, "Renderer: %s", bgfx::getRendererName(bgfx::getRendererType()));
        bgfx::dbgTextPrintf(1, 3, 0x0f, "FPS: %.1f", m_fps);
        bgfx::dbgTextPrintf(1, 4, 0x0f, "Draw calls: %u | Meshes: %u | Shadows: Active", m_stats.drawCalls, m_stats.meshDraws);
        bgfx::dbgTextPrintf(1, 5, 0x0f, "Move: WASD/QE | Look: hold RMB | Fast: Shift | Quit: Esc");
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

        if(!valid(m_handles->meshProgram) || !valid(m_handles->skyProgram) || !valid(m_handles->shadowProgram))
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

    void Renderer::setPbrUniforms(const Material& material)
    {
        const float tint[] = { material.baseColor.r, material.baseColor.g, material.baseColor.b, material.baseColor.a };
        const float materialData[] = { material.metallic, material.roughness, material.emissive, 0.0f };
        const float lightDirection[] = { m_light.direction.x, m_light.direction.y, m_light.direction.z, 0.0f };
        const float lightColor[] = { m_light.color.r, m_light.color.g, m_light.color.b, m_light.intensity };
        const float cameraData[] = { m_activeCamera.position.x, m_activeCamera.position.y, m_activeCamera.position.z, 1.15f };
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
