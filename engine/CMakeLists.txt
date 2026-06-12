find_package(SDL2 CONFIG REQUIRED)
find_package(bgfx CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

add_library(ArcEngine STATIC
    include/Arc/Application.hpp
    include/Arc/Engine.hpp
    src/Application.cpp
)

add_library(Arc::Engine ALIAS ArcEngine)

target_include_directories(ArcEngine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(ArcEngine PUBLIC cxx_std_20)

target_compile_definitions(ArcEngine
    PRIVATE
        SDL_MAIN_HANDLED
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        NOGDI
)

target_link_libraries(ArcEngine
    PRIVATE
        SDL2::SDL2
        bgfx::bgfx
        Vulkan::Vulkan
)

if(MSVC)
    target_compile_options(ArcEngine PRIVATE /W4 /permissive- /EHsc)
endif()

set_target_properties(ArcEngine PROPERTIES FOLDER "Engine")
