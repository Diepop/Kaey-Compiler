#pragma once
#include "Kaey/Ast/IObject.hpp"

namespace Kaey::Ast
{
    struct Statement : Context, Variant<Statement,
        struct BlockStatement,
        struct ExpressionStatement,
        struct FunctionDeclaration,
        struct IfStatement,
        struct VariableDeclaration,
        struct TupleVariableDeclaration,
        struct ConstructorDeclaration,
        struct WhileStatement,
        struct ForStatement,
        struct SwitchStatement,
        struct ClassDeclaration,
        struct FieldDeclaration,
        struct ParameterDeclaration,
        struct MethodDeclaration,
        struct AttributeDeclaration,
        struct DelegateInitializerStatement,
        struct FieldInitializerStatement
    >
    {
        using Context::Context;
        ArrayView<AttributeDeclaration*> Attributes() const { return attributes; }
        void AddAttribute(vector<Expression*> exprs);
        Statement* AsStatement() override { return this; }
    private:
        vector<AttributeDeclaration*> attributes;
    };

}
