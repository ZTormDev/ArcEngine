add_executable(ArcGame
    src/SandboxGame.cpp
)

target_link_libraries(ArcGame PRIVATE Arc::Engine)

if(TARGET ArcShaders)
    add_dependencies(ArcGame ArcShaders)
endif()

add_custom_command(TARGET ArcGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_BINARY_DIR}/generated/shaders"
        "$<TARGET_FILE_DIR:ArcGame>/shaders"
)

target_compile_features(ArcGame PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(ArcGame PRIVATE /W4 /permissive- /EHsc)
endif()

set_target_properties(ArcGame PROPERTIES FOLDER "Game")
