cmake_minimum_required(VERSION 3.16)
project(Veldanava VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_SHARED_LIBS "Build True_Dragon_Core as shared library" ON)

add_subdirectory(Veldanava/compiler)

add_executable(veldanc Veldanava/compiler/src/main.cpp)
target_link_libraries(veldanc PRIVATE True_Dragon_Core)

add_subdirectory(Veldora)
