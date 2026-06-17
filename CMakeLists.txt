cmake_minimum_required(VERSION 3.16)
project(Veldanava VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(veldanc
    src/main.cpp
    src/frontend/lexer/lexer.cpp
    src/frontend/parser/parser.cpp
    src/frontend/module/module_loader.cpp
    src/middle/ownership/ownership.cpp
    src/middle/typechecker/typechecker.cpp
    src/backend/codegen/codegen.cpp
    src/backend/vm/vm.cpp
)

target_include_directories(veldanc PRIVATE ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/frontend
    ${CMAKE_SOURCE_DIR}/src/middle
    ${CMAKE_SOURCE_DIR}/src/backend)