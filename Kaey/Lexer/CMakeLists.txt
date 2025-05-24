set(LexerDir ${CMAKE_CURRENT_SOURCE_DIR}/)

set(LexerSources 
    "DeclareToken.hpp"
    "KaeyLexer.hpp" "KaeyLexer.cpp"
)

list(TRANSFORM LexerSources PREPEND ${LexerDir})

add_library(KaeyLexer STATIC ${LexerSources})
target_include_directories(KaeyLexer PRIVATE ${LexerDir})

target_precompile_headers(KaeyLexer REUSE_FROM PCH)
