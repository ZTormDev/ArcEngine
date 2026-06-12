#include <Arc/Engine.hpp>

#include <cstddef>
#include <iostream>

class SandboxGame final : public Arc::Application
{
public:
    SandboxGame()
        : Arc::Application({ "Arc Engine Sandbox", 1280, 720, true })
    {
    }

private:
    void onStart() override
    {
        m_camera.camera().position = { 0.0f, 2.4f, 9.5f };
        m_camera.camera().pitch = Arc::radians(-10.0f);

        buildScene();

        std::cout << "ArcGame started. Hold right mouse button to look. WASD/QE to move.\n";
    }

    void onUpdate(float deltaSeconds) override
    {
        setMouseCaptured(input().isMouseButtonDown(Arc::MouseButton::Right));
        m_camera.update(input(), deltaSeconds);

        m_cubeRotation += deltaSeconds * 0.6f;
        m_scene.objects()[m_animatedCubeIndex].transform.rotation = { m_cubeRotation * 0.4f, m_cubeRotation, 0.0f };
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

        m_scene.addCube("Back Wall", { { 0.0f, 2.0f, -6.2f }, {}, { 8.0f, 2.0f, 0.18f } }, warmWall, false);
        m_scene.addCube("Left Wall", { { -8.0f, 2.0f, 0.0f }, {}, { 0.18f, 2.0f, 6.2f } }, darkConcrete, false);
        m_scene.addCube("Right Wall", { { 8.0f, 2.0f, 0.0f }, {}, { 0.18f, 2.0f, 6.2f } }, darkConcrete, false);
        m_scene.addCube("Left Pillar", { { -5.4f, 0.9f, 3.8f }, {}, { 0.38f, 0.9f, 0.38f } }, warmWall, false);
        m_scene.addCube("Right Pillar", { { 5.4f, 0.9f, 3.8f }, {}, { 0.38f, 0.9f, 0.38f } }, warmWall, false);

        m_animatedCubeIndex = m_scene.objects().size();
        m_scene.addCube("Animated Blue Cube", { { 0.0f, 1.05f, 0.0f }, {}, { 1.0f, 1.0f, 1.0f } }, bluePlastic);
        m_scene.addCube("Rough Red Box", { { -3.5f, 0.65f, -1.6f }, { 0.0f, 0.35f, 0.0f }, { 0.7f, 0.65f, 0.7f } }, roughRed);
        m_scene.addCube("Tall Metal Block", { { 3.1f, 1.35f, -2.4f }, { 0.0f, -0.25f, 0.0f }, { 0.65f, 1.35f, 0.65f } }, brushedMetal);
        m_scene.addSphere("Metal Sphere", { { -1.9f, 0.75f, 2.2f }, {}, { 0.75f, 0.75f, 0.75f } }, brushedMetal);
        m_scene.addSphere("Ceramic Sphere", { { 2.0f, 0.65f, 1.8f }, {}, { 0.65f, 0.65f, 0.65f } }, whiteCeramic);
        m_scene.addSphere("Rubber Sphere", { { 4.1f, 0.5f, 1.0f }, {}, { 0.5f, 0.5f, 0.5f } }, blackRubber);
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
    std::size_t m_animatedCubeIndex = 0;
};

int main()
{
    SandboxGame game;
    return game.run();
}
