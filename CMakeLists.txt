set(LLVM_LINK_COMPONENTS
    Support
)

set(TOOL_NAME PupilReflTool)

add_compile_definitions(CMAKE_DEF_PREFLTOOL_DEFAULT=L\"${CMAKE_CURRENT_SOURCE_DIR}\")

set(HEAD
    Generator.h
    Visitor.h
    Attributes.h
    CxxRecord.h
    PDeclNodes.inc
)

set(SRC
    Generator.cpp
    Visitor.cpp
    main.cpp
)

add_clang_executable(${TOOL_NAME}
    ${HEAD} ${SRC}
)

target_link_libraries(${TOOL_NAME}
    PRIVATE
    clangAST
    clangBasic
    clangFrontend
    clangTooling
)