# Tests: plain C++ assert harnesses via CTest. Zero new dependencies (§50):
# no GTest/Catch — the project vendors only nlohmann/json (header-only),
# which the skill-database test reuses. Each test is a standalone
# executable returning nonzero on the first failed expectation.
cmake_minimum_required(VERSION 3.20)

function(hsr_add_test name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/ui/src
    )
    target_compile_features(${name} PRIVATE cxx_std_17)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endfunction()

# Phase 1: skill-data parsing (toughness/energy/target-type/Talent mapping).
hsr_add_test(test_skilldb
    test_skilldb.cpp
    ${CMAKE_SOURCE_DIR}/ui/src/data/SkillDatabase.cpp
)

# Phase 2: Blast/AoE/Bounce splash + single-target preservation.
hsr_add_test(test_multitarget
    test_multitarget.cpp
    ${CMAKE_SOURCE_DIR}/src/simulation/SimulationEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/simulation/DamageCalculator.cpp
)

# Phase 4.3: multi-layered toughness and Exo-Toughness (direct config).
hsr_add_test(test_bars
    test_bars.cpp
    ${CMAKE_SOURCE_DIR}/src/simulation/SimulationEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/simulation/DamageCalculator.cpp
)

# Phase 3: Eidolon rules, E-gated scaling, DoT base-chance defaults.
hsr_add_test(test_eidolons
    test_eidolons.cpp
    ${CMAKE_SOURCE_DIR}/ui/src/data/CharacterDatabase.cpp
    ${CMAKE_SOURCE_DIR}/ui/src/data/SkillDatabase.cpp
    ${CMAKE_SOURCE_DIR}/ui/src/data/LightConeDatabase.cpp
    ${CMAKE_SOURCE_DIR}/ui/src/data/Relicsetdatabase.cpp
    ${CMAKE_SOURCE_DIR}/ui/src/data/LoadoutResolver.cpp
)

# Phase 4.2: break-DoT application and ticking.
hsr_add_test(test_dots
    test_dots.cpp
    ${CMAKE_SOURCE_DIR}/src/simulation/SimulationEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/simulation/DamageCalculator.cpp
)
# Phase 4.1: enemy offense targeting (DB overlay + engine splash).
# EnemyDatabase uses raylib's TraceLog, so this test links the vendored
# raylib (no new dependency). No other test needs it.
hsr_add_test(test_enemyaoe
    test_enemyaoe.cpp
    ${CMAKE_SOURCE_DIR}/ui/src/data/EnemyDatabase.cpp
    ${CMAKE_SOURCE_DIR}/src/simulation/SimulationEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/simulation/DamageCalculator.cpp
)
target_include_directories(test_enemyaoe PRIVATE ${CMAKE_SOURCE_DIR}/raylib/src)
target_link_libraries(test_enemyaoe PRIVATE raylib)
if(WIN32)
    target_link_libraries(test_enemyaoe PRIVATE winmm gdi32 opengl32)
endif()

# Phase 2: traces + LC passive opt-ins and resolver notes.
hsr_add_test(test_passives
    test_passives.cpp
    ${CMAKE_SOURCE_DIR}/ui/src/data/CharacterDatabase.cpp
    ${CMAKE_SOURCE_DIR}/ui/src/data/SkillDatabase.cpp
    ${CMAKE_SOURCE_DIR}/ui/src/data/LightConeDatabase.cpp
    ${CMAKE_SOURCE_DIR}/ui/src/data/Relicsetdatabase.cpp
    ${CMAKE_SOURCE_DIR}/ui/src/data/LoadoutResolver.cpp
)
