

project(vIED)

add_executable(${PROJECT_NAME} src/main.cpp)



target_link_libraries(${PROJECT_NAME}
    protection
    sniffer
    tools
    nlohmann_json::nlohmann_json
    goose
)

target_include_directories( ${PROJECT_NAME}
    PUBLIC 
        ${PROJECT_BINARY_DIR}
        ${PROJECT_SOURCE_DIR}/include
)

# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/files
#     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})