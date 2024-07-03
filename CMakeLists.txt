


cmake_minimum_required(VERSION 3.5)

project(Virtual-IED)

include_directories(${FFTW3_INCLUDE_DIR})

# Add sub directories
add_subdirectory(src)
add_subdirectory(src/protection)
add_subdirectory(src/sniffer)