cmake_minimum_required(VERSION 3.22)
project(BuzzFuzz VERSION 0.1.0)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_library(circuit STATIC Source/circuit/Circuit.cpp Source/circuit/BuzzFuzz.cpp)
target_compile_features(circuit PUBLIC cxx_std_17)

# checks the solver against ngspice (tools/compare.py)
add_executable(verify tools/verify.cpp)
target_link_libraries(verify PRIVATE circuit)
