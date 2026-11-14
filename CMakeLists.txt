cmake_minimum_required(VERSION 3.16)
project(uart_telemetry_protocol_lab LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(telemetry STATIC src/protocol.cpp)
target_include_directories(telemetry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MSVC)
  target_compile_options(telemetry PRIVATE /W4)
else()
  target_compile_options(telemetry PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

add_executable(telemetry_demo src/main.cpp)
target_link_libraries(telemetry_demo PRIVATE telemetry)

include(CTest)
if(BUILD_TESTING)
  add_executable(telemetry_tests tests/test_protocol.cpp)
  target_link_libraries(telemetry_tests PRIVATE telemetry)
  add_test(NAME telemetry_tests COMMAND telemetry_tests)
endif()
