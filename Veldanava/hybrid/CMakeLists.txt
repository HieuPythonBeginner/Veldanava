cmake_minimum_required(VERSION 3.16)
project(VeldanavaHybrid VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(veldanc_hybrid_opt
    src/main.cpp
    ../compiler/src/frontend/lexer/lexer.cpp
    ../compiler/src/frontend/parser/parser.cpp
    src/backend/hybrid_opt/hybrid_opt.cpp
)

target_include_directories(veldanc_hybrid_opt PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/backend
    ${CMAKE_SOURCE_DIR}/../compiler/src
    ${CMAKE_SOURCE_DIR}/../compiler/src/frontend
    ${CMAKE_SOURCE_DIR}/../compiler/src/middle
)
