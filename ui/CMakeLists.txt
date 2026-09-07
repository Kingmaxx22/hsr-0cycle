add_executable(hsr_ui
    src/main.cpp
    src/App.cpp
    src/assets/AssetManager.cpp
    src/data/CharacterDatabase.cpp
    src/data/LoadoutResolver.cpp
    src/data/Relicsetdatabase.cpp
    src/data/LightConeDatabase.cpp
    src/data/EnemyDatabase.cpp
    src/screens/TeamBuilderScreen.cpp
    src/screens/CharactersScreen.cpp
    src/screens/Relicrosterscreen.cpp
    src/screens/Reliceditorscreen.cpp
    src/screens/LightConeScreen.cpp
    src/screens/EnemiesScreen.cpp
    src/screens/SimulationScreen.cpp
    ../src/simulation/SimulationEngine.cpp
    ../src/simulation/EngineBridge.cpp
    ../src/simulation/DamageCalculator.cpp
    src/widgets/Button.cpp
    src/widgets/Panel.cpp
    src/widgets/CharacterCard.cpp
    src/widgets/Sidebar.cpp
)

# Find Python3 for the EngineBridge
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

target_include_directories(hsr_ui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/raylib/src
    ${CMAKE_SOURCE_DIR}/src
    ${Python3_INCLUDE_DIRS}
)

target_compile_definitions(hsr_ui PRIVATE
    HSR_ENGINE_DATA_DIR="${CMAKE_SOURCE_DIR}/engine/hsr_engine/data"
)

target_link_libraries(hsr_ui PRIVATE
    raylib
    ${Python3_LIBRARIES}
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(hsr_ui PRIVATE
        winmm
        gdi32
        opengl32
    )
endif()

add_custom_command(TARGET hsr_ui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/ui/assets"
        "$<TARGET_FILE_DIR:hsr_ui>/assets"
)
