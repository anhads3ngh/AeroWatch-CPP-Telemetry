cmake_minimum_required(VERSION 3.16)
project(AeroWatch VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(aerowatch_core
    src/telemetry.cpp
    src/monitor.cpp
    src/simulator.cpp
)
target_include_directories(aerowatch_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(aerowatch_core PRIVATE /W4)
else()
    target_compile_options(aerowatch_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(aerowatch src/main.cpp)
target_link_libraries(aerowatch PRIVATE aerowatch_core)

enable_testing()
add_executable(aerowatch_tests tests/test_main.cpp)
target_link_libraries(aerowatch_tests PRIVATE aerowatch_core)

set(AEROWATCH_TEST_CASES
    parser_valid
    parser_roundtrip
    parser_missing_field
    parser_extra_field
    parser_invalid_coordinate
    parser_nan
    parser_negative_battery
    parser_invalid_id
    parser_zero_timestamp
    monitor_nominal
    monitor_warning_battery
    monitor_critical_link
    monitor_offline
    monitor_duplicate_rejection
    monitor_out_of_order
    monitor_vehicle_isolation
    monitor_smoothing
    monitor_invalid_configuration
    simulator_determinism
    simulator_fault_injection
)
foreach(test_case IN LISTS AEROWATCH_TEST_CASES)
    add_test(NAME ${test_case} COMMAND aerowatch_tests --case ${test_case})
endforeach()
