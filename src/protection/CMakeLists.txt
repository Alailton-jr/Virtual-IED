

project(protection)


add_library(${PROJECT_NAME}
    src/pioc_phase.cpp
    src/pioc_neutral.cpp 
    src/ptoc_phase.cpp
    src/ptoc_neutral.cpp 
)

target_include_directories( ${PROJECT_NAME}
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/include
        tools
)