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
