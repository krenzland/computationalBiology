cmake_minimum_required(VERSION 3.0)
project(simulation)

file(GLOB_RECURSE sources simulation.cpp)

add_executable(simulation ${sources} ${data})

# just for example add some compiler flags
target_compile_options(simulation PUBLIC -std=c++14 -Wall -pedantic -O3 -march=native)

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()
