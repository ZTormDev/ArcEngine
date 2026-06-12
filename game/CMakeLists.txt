add_executable(ArcGame
    src/SandboxGame.cpp
)

target_link_libraries(ArcGame PRIVATE Arc::Engine)

target_compile_features(ArcGame PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(ArcGame PRIVATE /W4 /permissive- /EHsc)
endif()

set_target_properties(ArcGame PROPERTIES FOLDER "Game")
