#include <Arc/Engine.hpp>

#include <cstddef>
#include <iostream>
#include <cstdlib>
#include <ctime>

class SandboxGame final : public Arc::Application
{
public:
    SandboxGame()
        : Arc::Application({ "Arc Engine Sandbox", 1280, 720, true })
    {
    }

private:
    void testAllocators()
    {
        std::cout << "\n--- Testing Custom Allocators ---\n";
        
        // Linear Allocator
        alignas(16) char linearBuffer[1024];
        Arc::LinearAllocator linear(sizeof(linearBuffer), linearBuffer);
        int* linearInt = static_cast<int*>(linear.allocate(sizeof(int), alignof(int)));
        if (linearInt) {
            *linearInt = 42;
            std::cout << "Linear Allocator: Success, value = " << *linearInt << ", used memory = " << linear.getUsedMemory() << " bytes\n";
        }

        // Stack Allocator
        alignas(16) char stackBuffer[1024];
        Arc::StackAllocator stack(sizeof(stackBuffer), stackBuffer);
        auto marker = stack.getMarker();
        double* stackDouble = static_cast<double*>(stack.allocate(sizeof(double), alignof(double)));
        if (stackDouble) {
            *stackDouble = 3.14159;
            std::cout << "Stack Allocator: Success, value = " << *stackDouble << ", used memory = " << stack.getUsedMemory() << " bytes\n";
        }
        stack.freeToMarker(marker);
        std::cout << "Stack Allocator: Rolled back marker, used memory = " << stack.getUsedMemory() << " bytes\n";

        // Pool Allocator
        alignas(16) char poolBuffer[1024];
        struct TempObj { char data[16]; };
        Arc::PoolAllocator pool(sizeof(TempObj), alignof(TempObj), sizeof(poolBuffer), poolBuffer);
        void* obj1 = pool.allocate(sizeof(TempObj), alignof(TempObj));
        void* obj2 = pool.allocate(sizeof(TempObj), alignof(TempObj));
        (void)obj2;
        std::cout << "Pool Allocator: Allocated 2 blocks of size 16, used memory = " << pool.getUsedMemory() << " bytes\n";
        pool.deallocate(obj1);
        std::cout << "Pool Allocator: Deallocated 1 block, used memory = " << pool.getUsedMemory() << " bytes\n";

        // Frame Allocator
        int* frameInt = Arc::FrameAllocator::make<int>(1337);
        if (frameInt) {
            std::cout << "Frame Allocator: Successfully allocated transient int with value = " << *frameInt << "\n";
        }
        std::cout << "---------------------------------\n\n";
    }

    void onStart() override
    {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));

        m_camera.camera().position = { 0.0f, 2.4f, 9.5f };
        m_camera.camera().pitch = Arc::radians(-10.0f);

        buildScene();

        // Run allocator validation tests
        testAllocators();

        // Register custom console command
        Arc::Console::instance().registerCommand("spawn_cube", [this](const std::vector<std::string>& args) {
            (void)args;
            float rX = static_cast<float>(std::rand() % 100) / 10.0f - 5.0f;
            float rY = static_cast<float>(std::rand() % 40) / 10.0f + 0.5f;
            float rZ = static_cast<float>(std::rand() % 100) / 10.0f - 5.0f;

            const Arc::Material randomColor{
                { static_cast<float>(std::rand() % 100) / 100.0f,
                  static_cast<float>(std::rand() % 100) / 100.0f,
                  static_cast<float>(std::rand() % 100) / 100.0f,
                  1.0f },
                0.0f,
                0.5f,
                0.0f
            };

            m_scene.addCube("Spawned Cube", { { rX, rY, rZ }, {}, { 0.5f, 0.5f, 0.5f } }, randomColor);
            std::cout << "Spawned new cube at position: (" << rX << ", " << rY << ", " << rZ << ")\n";
        });

        std::cout << "ArcGame started. Hold right mouse button to look. WASD/QE to move.\n";
        std::cout << "Developer Console active. Type commands in this console window (e.g. 'help', 'spawn_cube', 'sys_speed 0.5', 'sys_fps_limit 60').\n";
    }

    void onUpdate(float deltaSeconds) override
    {
        setMouseCaptured(input().isMouseButtonDown(Arc::MouseButton::Right));
        m_camera.update(input(), deltaSeconds);

        m_cubeRotation += deltaSeconds * 0.6f;
        
        // Rotate the parent cube via its TransformComponent
        if (m_scene.registry().isValid(m_animatedCubeEntity))
        {
            auto& tc = m_scene.registry().get<Arc::TransformComponent>(m_animatedCubeEntity);
            tc.rotation = { m_cubeRotation * 0.4f, m_cubeRotation, 0.0f };
        }

        // Recursively compute the parent-child transform hierarchy
        m_scene.updateTransforms();
    }

    void onRender(Arc::Renderer& renderer) override
    {
        renderer.beginScene(m_camera.camera());
        renderer.setDirectionalLight(m_sun);
        renderer.drawSkybox(m_skybox);
        renderer.drawGround(16.0f, m_groundMaterial);
        m_scene.render(renderer);
        renderer.drawSun(m_sun, 80.0f, 2.2f);
    }

    void onShutdown() override
    {
        setMouseCaptured(false);
        std::cout << "ArcGame shutdown.\n";
    }

    void buildScene()
    {
        const Arc::Material darkConcrete{ { 0.30f, 0.31f, 0.32f, 1.0f }, 0.0f, 0.82f, 0.0f };
        const Arc::Material warmWall{ { 0.58f, 0.54f, 0.48f, 1.0f }, 0.0f, 0.72f, 0.0f };
        const Arc::Material bluePlastic{ { 0.20f, 0.42f, 0.95f, 1.0f }, 0.0f, 0.34f, 0.0f };
        const Arc::Material roughRed{ { 0.85f, 0.24f, 0.16f, 1.0f }, 0.0f, 0.70f, 0.0f };
        const Arc::Material brushedMetal{ { 0.80f, 0.76f, 0.68f, 1.0f }, 1.0f, 0.28f, 0.0f };
        const Arc::Material blackRubber{ { 0.015f, 0.014f, 0.013f, 1.0f }, 0.0f, 0.92f, 0.0f };
        const Arc::Material whiteCeramic{ { 0.92f, 0.90f, 0.86f, 1.0f }, 0.0f, 0.18f, 0.0f };

        // Emissive Glowing Materials for showcasing Post-Processing Bloom
        const Arc::Material neonGreen{ { 0.10f, 0.95f, 0.20f, 1.0f }, 0.0f, 0.5f, 10.0f };
        const Arc::Material neonOrange{ { 0.98f, 0.50f, 0.05f, 1.0f }, 0.0f, 0.5f, 8.0f };
        const Arc::Material neonPink{ { 0.95f, 0.15f, 0.60f, 1.0f }, 0.0f, 0.5f, 12.0f };

        m_scene.addCube("Back Wall", { { 0.0f, 2.0f, -6.2f }, {}, { 8.0f, 2.0f, 0.18f } }, warmWall);
        m_scene.addCube("Left Wall", { { -8.0f, 2.0f, 0.0f }, {}, { 0.18f, 2.0f, 6.2f } }, darkConcrete);
        m_scene.addCube("Right Wall", { { 8.0f, 2.0f, 0.0f }, {}, { 0.18f, 2.0f, 6.2f } }, darkConcrete);
        m_scene.addCube("Left Pillar", { { -5.4f, 0.9f, 3.8f }, {}, { 0.38f, 0.9f, 0.38f } }, warmWall);
        m_scene.addCube("Right Pillar", { { 5.4f, 0.9f, 3.8f }, {}, { 0.38f, 0.9f, 0.38f } }, warmWall);

        // Spawn Parent Cube
        m_animatedCubeEntity = m_scene.addCube("Animated Blue Cube", { { 0.0f, 1.5f, 0.0f }, {}, { 1.2f, 1.2f, 1.2f } }, bluePlastic);
        
        // Spawn Child Sphere and link it to the Parent Cube
        m_childSphereEntity = m_scene.addSphere("Orbiting Child Sphere", { { 2.2f, 0.0f, 0.0f }, {}, { 0.5f, 0.5f, 0.5f } }, whiteCeramic);
        m_scene.setParent(m_childSphereEntity, m_animatedCubeEntity);

        m_scene.addCube("Rough Red Box", { { -3.5f, 0.65f, -1.6f }, { 0.0f, 0.35f, 0.0f }, { 0.7f, 0.65f, 0.7f } }, roughRed);
        m_scene.addCube("Tall Metal Block", { { 3.1f, 1.35f, -2.4f }, { 0.0f, -0.25f, 0.0f }, { 0.65f, 1.35f, 0.65f } }, brushedMetal);
        m_scene.addSphere("Metal Sphere", { { -1.9f, 0.75f, 2.2f }, {}, { 0.75f, 0.75f, 0.75f } }, brushedMetal);
        m_scene.addSphere("Ceramic Sphere", { { 2.0f, 0.65f, 1.8f }, {}, { 0.65f, 0.65f, 0.65f } }, whiteCeramic);
        m_scene.addSphere("Rubber Sphere", { { 4.1f, 0.5f, 1.0f }, {}, { 0.5f, 0.5f, 0.5f } }, blackRubber);

        // Add Glowing Neon Objects to the scene
        m_scene.addCube("Neon Green Cube", { { -5.4f, 2.1f, 3.8f }, {}, { 0.4f, 0.4f, 0.4f } }, neonGreen);
        m_scene.addSphere("Neon Orange Sphere", { { 5.4f, 2.1f, 3.8f }, {}, { 0.4f, 0.4f, 0.4f } }, neonOrange);
        m_scene.addCube("Neon Pink Cube", { { 0.0f, 1.2f, -3.5f }, {}, { 0.5f, 0.5f, 0.5f } }, neonPink);
    }

    Arc::FreeCamera m_camera;
    Arc::Scene m_scene;
    Arc::Material m_groundMaterial{ { 0.46f, 0.45f, 0.41f, 1.0f }, 0.0f, 0.88f, 0.0f };
    Arc::DirectionalLight m_sun{
        Arc::normalize(Arc::Vec3{ -0.35f, -1.0f, -0.2f }),
        { 1.0f, 0.92f, 0.68f, 1.0f },
        7.0f
    };
    Arc::Skybox m_skybox{
        { 0.62f, 0.74f, 0.92f, 1.0f },
        { 0.06f, 0.13f, 0.28f, 1.0f },
        { 1.0f, 0.68f, 0.28f, 1.0f }
    };
    float m_cubeRotation = 0.0f;
    Arc::Entity m_animatedCubeEntity = Arc::NullEntity;
    Arc::Entity m_childSphereEntity = Arc::NullEntity;
};

int main()
{
    SandboxGame game;
    return game.run();
}
