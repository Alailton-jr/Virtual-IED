

project(protection)


add_library(${PROJECT_NAME}
    src/pioc.cpp
    src/ptoc.cpp
    src/pdis.cpp
    src/trip_time_opt.cpp
)

target_include_directories( ${PROJECT_NAME}
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/include
        tools
)