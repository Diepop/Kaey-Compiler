cmake_minimum_required(VERSION 3.5)

set(PCHDir ${CMAKE_CURRENT_SOURCE_DIR}/)

set(PCHHeaders "pch.hpp")
set(PCHSources "pch.cpp")

list(TRANSFORM PCHHeaders PREPEND ${PCHDir})
list(TRANSFORM PCHSources PREPEND ${PCHDir})

find_package(LLVM CONFIG REQUIRED)

add_library(PCH STATIC ${PCHSources})

target_include_directories(PCH PUBLIC ${LLVM_INCLUDE_DIRS})

target_link_libraries(PCH PUBLIC ${llvm_libs})

target_link_libraries(PCH PUBLIC
    LTO LLVMMC LLVMLTO LLVMMCA
    LLVMOrcJIT LLVMX86AsmParser LLVMX86CodeGen
    LLVMX86Desc LLVMX86Disassembler LLVMX86Info
)

target_precompile_headers(PCH PUBLIC ${PCHHeaders})
