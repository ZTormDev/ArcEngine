add_executable(ArcGame

    Source/Main.cpp
)

target_link_libraries(ArcGame PRIVATE

    ArcEngine
)

target_compile_definitions(ArcGame PRIVATE

    SDL_MAIN_HANDLED
)