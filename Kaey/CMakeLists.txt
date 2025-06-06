set(KaeyDir ${CMAKE_CURRENT_SOURCE_DIR}/)

include_directories("${CMAKE_SOURCE_DIR}/Kaey")

add_subdirectory("Lexer")
add_subdirectory("Parser")
add_subdirectory("Ast")
add_subdirectory("Llvm")

set(KaeySources
	"Utility.hpp"
	"Operators.hpp"
	"Coroutine.hpp"
	"Compiler.hpp" "Compiler.cpp"
)

list(TRANSFORM KaeySources PREPEND ${KaeyDir})

add_library(Kaey STATIC ${KaeySources})

target_link_libraries(Kaey PUBLIC KaeyLexer KaeyParser KaeyLlvm KaeyAst)

target_precompile_headers(Kaey REUSE_FROM PCH)
