cmake_minimum_required (VERSION 3.0)

set(ParserDir "${CMAKE_SOURCE_DIR}/Kaey/Parser/")

set(ParserSources 
    "KaeyParser.hpp" "KaeyParser.cpp"
)

list(TRANSFORM ParserSources PREPEND ${ParserDir})

add_library(KaeyParser STATIC ${ParserSources})
target_include_directories(KaeyParser PRIVATE ${ParserDir})
target_precompile_headers(KaeyParser REUSE_FROM KaeyLexer)
