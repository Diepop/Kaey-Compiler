cmake_minimum_required (VERSION 3.0)

set(LlvmDir "${CMAKE_SOURCE_DIR}/Kaey/Llvm/")

set(LlvmSources 
    "IObject.hpp"
    "JitCompiler.hpp" "JitCompiler.cpp"
    "Expression.hpp" "Expression.cpp"
    
    "ModuleContext.hpp" "ModuleContext.cpp"
    "Function.hpp" "Function.cpp"
    
    "Type.hpp" "Type.cpp"
    "Types.hpp" "Types.cpp"
)

list(TRANSFORM LlvmSources PREPEND ${LlvmDir})

find_package(LLVM CONFIG REQUIRED)

add_library(KaeyLlvm STATIC ${LlvmSources})

target_link_libraries(KaeyLlvm PUBLIC
    LTO LLVMMC LLVMLTO LLVMMCA
    LLVMOrcJIT LLVMX86AsmParser LLVMX86CodeGen
    LLVMX86Desc LLVMX86Disassembler LLVMX86Info
)

target_include_directories(KaeyLlvm PRIVATE ${LlvmDir})
target_precompile_headers(KaeyLlvm REUSE_FROM KaeyLexer)
