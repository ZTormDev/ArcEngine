add_library(ArcEngine STATIC

    Arc/Core/Application.cpp
    Arc/Core/Log.cpp

    Arc/Platform/Window.cpp

    Arc/Renderer/Renderer.cpp
    Arc/Renderer/Camera/FreeCamera.cpp
)

set(FILAMENT_ROOT "${CMAKE_SOURCE_DIR}/ThirdParty/Filament")
set(FILAMENT_LIB_DIR "${FILAMENT_ROOT}/lib/x86_64/md")

target_include_directories(ArcEngine PUBLIC

    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FILAMENT_ROOT}/include
)

target_link_directories(ArcEngine PUBLIC

    ${FILAMENT_LIB_DIR}
)

target_link_libraries(ArcEngine PUBLIC

    SDL2::SDL2
    SDL2::SDL2main

    glm::glm
    EnTT::EnTT
    spdlog::spdlog

    filament
    backend
    utils
    filabridge
    geometry
    filaflat
    bluevk
    bluegl
    zstd
    smol-v

    opengl32
)

target_compile_definitions(ArcEngine PUBLIC

    SDL_MAIN_HANDLED
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

set(ARC_MATERIAL_SOURCE
    "${CMAKE_SOURCE_DIR}/Assets/Engine/Materials/ArcDefault.mat"
)

set(ARC_MATERIAL_OUTPUT
    "${CMAKE_SOURCE_DIR}/Assets/Engine/Materials/ArcDefault.filamat"
)

set(MATC_EXECUTABLE
    "${CMAKE_SOURCE_DIR}/ThirdParty/Filament/bin/matc.exe"
)

add_custom_command(
    OUTPUT ${ARC_MATERIAL_OUTPUT}
    COMMAND ${MATC_EXECUTABLE}
            -p desktop
            -a vulkan
            -o ${ARC_MATERIAL_OUTPUT}
            ${ARC_MATERIAL_SOURCE}
    DEPENDS ${ARC_MATERIAL_SOURCE}
    COMMENT "Compiling ArcDefault.mat to ArcDefault.filamat"
)

add_custom_target(ArcMaterials ALL
    DEPENDS ${ARC_MATERIAL_OUTPUT}
)

add_dependencies(ArcEngine ArcMaterials)