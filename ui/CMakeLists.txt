add_executable(hsr_ui
    src/main.cpp
    src/App.cpp
    src/assets/AssetManager.cpp
    src/data/CharacterDatabase.cpp
    src/data/RelicSetDatabase.cpp
    src/data/LightConeDatabase.cpp
    src/data/EnemyDatabase.cpp
    src/screens/TeamBuilderScreen.cpp
    src/screens/RelicRosterScreen.cpp
    src/screens/RelicEditorScreen.cpp
    src/screens/LightConeScreen.cpp
    src/screens/EnemiesScreen.cpp
    src/widgets/Button.cpp
    src/widgets/Panel.cpp
    src/widgets/CharacterCard.cpp
    src/widgets/Sidebar.cpp
)

target_include_directories(hsr_ui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/raylib/src
)

target_compile_definitions(hsr_ui PRIVATE
    HSR_ENGINE_DATA_DIR="${CMAKE_SOURCE_DIR}/engine/hsr_engine/data"
)

target_link_libraries(hsr_ui PRIVATE
    raylib
    winmm
    gdi32
    opengl32
)

add_custom_command(TARGET hsr_ui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/ui/assets"
        "$<TARGET_FILE_DIR:hsr_ui>/assets"
)
