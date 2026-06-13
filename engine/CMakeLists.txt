find_package(SDL2 CONFIG REQUIRED)
find_package(bgfx CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_path(STB_INCLUDE_DIRS "stb_image.h")

option(ARC_ENABLE_TRACY "Enable Tracy profiler integration" OFF)
if(ARC_ENABLE_TRACY)
    find_package(tracy CONFIG REQUIRED)
endif()

add_library(ArcEngine STATIC
    include/Arc/Application.hpp
    include/Arc/Camera.hpp
    include/Arc/Engine.hpp
    include/Arc/Input.hpp
    include/Arc/Math.hpp
    include/Arc/Mesh.hpp
    include/Arc/Renderer.hpp
    include/Arc/Scene.hpp
    include/Arc/AssetManager.hpp
    include/Arc/Memory.hpp
    include/Arc/Console.hpp
    include/Arc/Profiler.hpp
    src/AssetManager.cpp
    src/Application.cpp
    src/Camera.cpp
    src/Input.cpp
    src/Renderer.cpp
    src/Scene.cpp
    src/Memory.cpp
    src/Console.cpp
)

add_library(Arc::Engine ALIAS ArcEngine)

target_include_directories(ArcEngine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${STB_INCLUDE_DIRS}
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
        bgfx::bx
        bgfx::bgfx
        Vulkan::Vulkan
)

if(ARC_ENABLE_TRACY)
    target_link_libraries(ArcEngine PRIVATE tracy::tracy)
    target_compile_definitions(ArcEngine PUBLIC TRACY_ENABLE)
endif()

set(ARC_SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(ARC_SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/generated/shaders")

if(COMMAND bgfx_compile_shaders)
    bgfx_compile_shaders(
        TYPE VERTEX
        SHADERS
            "${ARC_SHADER_SOURCE_DIR}/vs_mesh.sc"
            "${ARC_SHADER_SOURCE_DIR}/vs_sky.sc"
            "${ARC_SHADER_SOURCE_DIR}/vs_shadow.sc"
            "${ARC_SHADER_SOURCE_DIR}/vs_post.sc"
        VARYING_DEF "${ARC_SHADER_SOURCE_DIR}/varying.def.sc"
        OUTPUT_DIR "${ARC_SHADER_OUTPUT_DIR}"
        OUT_FILES_VAR ARC_VERTEX_SHADER_OUTPUTS
        INCLUDE_DIRS "${ARC_SHADER_SOURCE_DIR}"
    )

    bgfx_compile_shaders(
        TYPE FRAGMENT
        SHADERS
            "${ARC_SHADER_SOURCE_DIR}/fs_mesh.sc"
            "${ARC_SHADER_SOURCE_DIR}/fs_sky.sc"
            "${ARC_SHADER_SOURCE_DIR}/fs_shadow.sc"
            "${ARC_SHADER_SOURCE_DIR}/fs_bloom_down.sc"
            "${ARC_SHADER_SOURCE_DIR}/fs_bloom_up.sc"
            "${ARC_SHADER_SOURCE_DIR}/fs_post.sc"
            "${ARC_SHADER_SOURCE_DIR}/fs_taa.sc"
            "${ARC_SHADER_SOURCE_DIR}/fs_dof.sc"
            "${ARC_SHADER_SOURCE_DIR}/fs_motion_blur.sc"
            "${ARC_SHADER_SOURCE_DIR}/fs_luminance_down.sc"
            "${ARC_SHADER_SOURCE_DIR}/fs_luminance_adapt.sc"
            "${ARC_SHADER_SOURCE_DIR}/fs_linear_depth.sc"
            "${ARC_SHADER_SOURCE_DIR}/fs_ssao.sc"
            "${ARC_SHADER_SOURCE_DIR}/fs_ssao_blur.sc"
            "${ARC_SHADER_SOURCE_DIR}/fs_ssr.sc"
            "${ARC_SHADER_SOURCE_DIR}/fs_ssgi.sc"
            "${ARC_SHADER_SOURCE_DIR}/fs_scene_combine.sc"
        VARYING_DEF "${ARC_SHADER_SOURCE_DIR}/varying.def.sc"
        OUTPUT_DIR "${ARC_SHADER_OUTPUT_DIR}"
        OUT_FILES_VAR ARC_FRAGMENT_SHADER_OUTPUTS
        INCLUDE_DIRS "${ARC_SHADER_SOURCE_DIR}"
    )

    add_custom_target(ArcShaders DEPENDS ${ARC_VERTEX_SHADER_OUTPUTS} ${ARC_FRAGMENT_SHADER_OUTPUTS})
    add_dependencies(ArcEngine ArcShaders)
else()
    message(FATAL_ERROR "BGFX shader tools are required. vcpkg.json enables bgfx[tools]; re-run CMake configure.")
endif()

if(MSVC)
    target_compile_options(ArcEngine PRIVATE /W4 /permissive- /EHsc)
endif()

set_target_properties(ArcEngine PROPERTIES FOLDER "Engine")
