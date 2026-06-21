cmake_minimum_required(VERSION 3.16)
project(Veldora VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Veldora
    src/main.cpp
    src/frontend/lexer/lexer.cpp
    src/frontend/lexer/lexer.h
    src/frontend/parser/parser.cpp
    src/frontend/parser/parser.h
    src/frontend/ast/ast.h
    src/backend/interpreter/interpreter.cpp
    src/backend/interpreter/interpreter.h
    src/backend/bridge/bridge.cpp
    src/backend/bridge/bridge.h
    src/backend/hybrid/hybrid.cpp
    src/backend/hybrid/hybrid.h
)

target_include_directories(Veldora PRIVATE
    ${CMAKE_SOURCE_DIR}/Veldora/src
    ${CMAKE_SOURCE_DIR}/Veldanava/compiler/public
)

target_link_libraries(Veldora PRIVATE True_Dragon_Core)
