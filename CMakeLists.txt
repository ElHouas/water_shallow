# This is the CMake script for compiling this folder.

cmake_minimum_required(VERSION 3.1...3.15)
project(water_shallow)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()

# Add compiler options.
add_definitions(-Wall -Wextra)

find_package(Boost QUIET)
find_package(MPI REQUIRED)

# Add source directory
aux_source_directory("./src" SRC_LIST)

# include_directories(SYSTEM ${MPI_INCLUDE_PATH})

# Add executable
add_executable(${PROJECT_NAME} ${SRC_LIST})
# target_link_libraries(${PROJECT_NAME}  ${MPI_C_LIBRARIES})