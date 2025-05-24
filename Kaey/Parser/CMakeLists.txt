set(ParserDir ${CMAKE_CURRENT_SOURCE_DIR}/)

set(ParserSources 
    "KaeyParser.hpp" "KaeyParser.cpp"
)

list(TRANSFORM ParserSources PREPEND ${ParserDir})

add_library(KaeyParser STATIC ${ParserSources})
target_include_directories(KaeyParser PRIVATE ${ParserDir})
target_precompile_headers(KaeyParser REUSE_FROM PCH)
