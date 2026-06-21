add_library(True_Dragon_Core SHARED
    src/frontend/lexer/lexer.cpp
    src/frontend/lexer/token.h
    src/frontend/lexer/keyword_table.h
    src/frontend/parser/parser.cpp
    src/frontend/ast/ast.h
    src/frontend/module/module_loader.cpp
    src/middle/typechecker/typechecker.cpp
    src/middle/ownership/ownership.cpp
    src/backend/codegen/codegen.cpp
    src/backend/vm/instruction.h
    src/backend/vm/vm.cpp
    public/vm_instruction.h
    public/vm_api.h
    public/bridge.h
    public/bridge.cpp
)

target_include_directories(True_Dragon_Core PUBLIC
    ${CMAKE_SOURCE_DIR}/Veldanava/compiler/src
    ${CMAKE_SOURCE_DIR}/Veldanava/compiler/src/frontend
    ${CMAKE_SOURCE_DIR}/Veldanava/compiler/src/middle
    ${CMAKE_SOURCE_DIR}/Veldanava/compiler/src/backend
    ${CMAKE_SOURCE_DIR}/Veldanava/compiler/public
)
