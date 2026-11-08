if(CMAKE_SYSTEM_NAME STREQUAL "Linux"
    AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$"
    AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
    AND CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL "9.4.0")
    include(${CMAKE_SOURCE_DIR}/cmake/ImportSFML.cmake)
else()
    include(FetchContent)
    FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.1.0
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM
    )
    set(SFML_USE_SYSTEM_DEPS OFF)
    FetchContent_MakeAvailable(SFML)

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(freetype PRIVATE harfbuzz)
    endif()
endif()

set(ECS_ENGINE_FILES
    ECSEngine.h
    Entity.h
    MathUtil.h
    Keyboard.h
)

set(ECS_COMPONENT_FILES
    Component.h
    CameraComponent.h
    CollisionComponent.h
    InputComponent.h
    LocationComponent.h
    ScoreComponent.h
    SpawnComponent.h
    SpriteComponent.h
)

set(ECS_SYSTEM_FILES
    CameraSystem.h
    ScoreSystem.h
    SpriteSystem.h
    CollisionSystem.h
    CollisionLateSystem.h
    SpawnSystem.h
    MovementSystem.h
    GravitySystem.h
    InputSystem.h
)

set(ECS_MANAGER_FILES
    EntityManager.h
    SoundManager.h SoundManager.cpp
    SpriteManager.h SpriteManager.cpp
    SystemManager.h
    WindowManager.h WindowManager.cpp
)

add_library(ECS STATIC
    ${ECS_ENGINE_FILES}
    ${ECS_COMPONENT_FILES}
    ${ECS_SYSTEM_FILES}
    ${ECS_MANAGER_FILES}
)

source_group("Engine" FILES ${ECS_ENGINE_FILES})
source_group("Components" FILES ${ECS_COMPONENT_FILES})
source_group("Systems" FILES ${ECS_SYSTEM_FILES})
source_group("Managers" FILES ${ECS_MANAGER_FILES})

add_executable(ECSTest EngineTest.cpp)

target_include_directories(ECS INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(ECS PRIVATE cxx_std_20)
target_compile_features(ECSTest PRIVATE cxx_std_20)

target_link_libraries(ECS PUBLIC SFML::Graphics SFML::Audio)
target_link_libraries(ECSTest PRIVATE ECS)

enable_warnings(ECS)
enable_warnings(ECSTest)
