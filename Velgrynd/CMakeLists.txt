cmake_minimum_required(VERSION 3.16)
project(Velgrynd VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(velgrynd_JIT
    src/main.cpp
    src/frontend/lexer/lexer.cpp
    src/frontend/parser/parser.cpp
    src/backend/jit/jit.cpp
)

target_include_directories(velgrynd_JIT PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/frontend
    ${CMAKE_SOURCE_DIR}/src/backend
)
