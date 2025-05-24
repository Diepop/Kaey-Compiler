set(LlvmDir ${CMAKE_CURRENT_SOURCE_DIR}/)

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

add_library(KaeyLlvm STATIC ${LlvmSources})

target_link_libraries(KaeyLlvm PUBLIC PCH)

target_include_directories(KaeyLlvm PRIVATE ${LlvmDir})
target_precompile_headers(KaeyLlvm REUSE_FROM PCH)
