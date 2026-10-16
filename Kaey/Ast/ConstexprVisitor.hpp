#pragma once
#include "Kaey/Ast/Module.hpp"

namespace Kaey::Ast
{
    struct ConstexprVisitor : Expression::Visitor<Expression*>
    {
        Expression* Visit(BooleanConstant* arg);

        Expression* Visit(CharacterConstant* arg);

        Expression* Visit(FloatConstant* arg);

        Expression* Visit(IntegerConstant* arg);

        Expression* Visit(VoidConstant* arg);

        Expression* Visit(TupleExpression* arg);

        Expression* Visit(BinaryExpression* arg);

        Expression* Visit(UnaryExpression* arg);

        explicit ConstexprVisitor(Context* ctx);

    private:
        Context* ctx;

    };

}
