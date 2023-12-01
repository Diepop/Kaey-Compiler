cmake_minimum_required (VERSION 3.0)

set(LexerDir "${CMAKE_SOURCE_DIR}/Kaey/Lexer/")

set(LexerSources 
    "DeclareToken.hpp"
    "KaeyLexer.hpp" "KaeyLexer.cpp"
)

list(TRANSFORM LexerSources PREPEND ${LexerDir})

add_library(KaeyLexer STATIC ${LexerSources})
target_include_directories(KaeyLexer PRIVATE ${LexerDir})

target_precompile_headers(KaeyLexer PUBLIC
    "${CMAKE_SOURCE_DIR}/pch.cpp"
    "${CMAKE_SOURCE_DIR}/pch.hpp"
)
