


cmake_minimum_required(VERSION 3.5)

project(Virtual-IED VERSION 1.0)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/out/build)

find_package(nlohmann_json 3.2.0 REQUIRED)

# Subdirectories
add_subdirectory(src/api) # API - Communication with the Configuration Tool
add_subdirectory(src/goose)
add_subdirectory(src/logic)
add_subdirectory(src/main)
add_subdirectory(src/protection)
add_subdirectory(src/sniffer)
add_subdirectory(src/tools)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/files
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

