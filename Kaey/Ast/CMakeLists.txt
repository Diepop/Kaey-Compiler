set(AstDir ${CMAKE_CURRENT_SOURCE_DIR}/)

set(AstSources
    "CreateAst.hpp" "CreateAst.cpp"
    "Module.hpp" "Module.cpp"

    "Expression.hpp" "Expression.cpp"
    "Expressions.hpp" "Expressions.cpp"

    "Statement.hpp" "Statement.cpp"
    "Statements.hpp" "Statements.cpp"

    "Type.hpp" "Type.cpp"
    "Types.hpp" "Types.cpp"
)

list(TRANSFORM AstSources PREPEND ${AstDir})

add_library(KaeyAst STATIC ${AstSources})
target_include_directories(KaeyAst PRIVATE ${AstDir})
target_precompile_headers(KaeyAst REUSE_FROM PCH)
