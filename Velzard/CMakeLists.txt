cmake_minimum_required(VERSION 3.16)
project(Velzard VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(velzard_Interpreter
    src/main.cpp
    src/frontend/lexer/lexer.cpp
    src/frontend/parser/parser.cpp
    src/backend/interpreter/interpreter.cpp
)

target_include_directories(velzard_Interpreter PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/frontend
    ${CMAKE_SOURCE_DIR}/src/backend
)
