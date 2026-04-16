
set(CMAKE_BUILD_TYPE Debug)

add_definitions(-DSIMPLIFY_EXPR)

add_llvm_executable(symrun main.cpp sym_run.cpp)
target_link_libraries(symrun PUBLIC ${llvm_libs} SvfLLVM comgraph)

target_compile_definitions(symrun PRIVATE SIMPLIFY_EXPR)
target_compile_options(symrun PRIVATE -fexceptions)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(symrun PRIVATE SEMSLICE_DEBUG)
endif()