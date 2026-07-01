cmake_minimum_required(VERSION 3.16)
project(Veldanava VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_SHARED_LIBS "Build True_Dragon_Core as shared library" ON)

add_subdirectory(Veldanava/compiler)

add_executable(veldanc Veldanava/compiler/src/main.cpp)
target_link_libraries(veldanc PRIVATE True_Dragon_Core)

add_executable(velandavac ${CMAKE_SOURCE_DIR}/velandavac.cpp)
target_include_directories(velandavac PRIVATE ${CMAKE_SOURCE_DIR})

add_executable(running_true_dragon_core ${CMAKE_SOURCE_DIR}/velandavac.cpp)
target_include_directories(running_true_dragon_core PRIVATE ${CMAKE_SOURCE_DIR})

add_executable(velrun ${CMAKE_SOURCE_DIR}/velandavac.cpp)
target_include_directories(velrun PRIVATE ${CMAKE_SOURCE_DIR})

add_subdirectory(Veldora)

install(TARGETS veldanc running_true_dragon_core velrun
    RUNTIME DESTINATION bin
)
