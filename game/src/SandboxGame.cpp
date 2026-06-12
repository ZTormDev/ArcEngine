#include <Arc/Engine.hpp>

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
        std::cout << "ArcGame started. Press Esc in the window to quit.\n";
    }

    void onUpdate(float deltaSeconds) override
    {
        (void)deltaSeconds;
    }

    void onShutdown() override
    {
        std::cout << "ArcGame shutdown.\n";
    }
};

int main()
{
    SandboxGame game;
    return game.run();
}
