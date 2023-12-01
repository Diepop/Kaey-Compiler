#pragma once
#include "Kaey/Ast/IObject.hpp"

namespace Kaey::Ast
{
    struct Expression : IObject, Variant<Expression,
        struct ArrayExpression,
        struct BooleanConstant,
        struct CharacterConstant,
        struct FloatConstant,
        struct IntegerConstant,
        struct TupleExpression,
        struct VoidConstant,
        struct BinaryExpression,
        struct UnaryExpression,
        struct Function,
        struct CallExpression,
        struct VariableReferenceExpression,
        struct ConstructExpression,
        struct ExternCAttribute,
        struct NewExpression,
        struct DeleteExpression,
        struct ReturnExpression,
        struct MemberAccessExpression,
        struct SubscriptOperatorExpression,
        struct TernaryExpression,
        struct SizeOfExpression,
        struct AlignOfExpression
    >
    {
        Expression(SystemModule* sys, Ast::Type* type);
        SystemModule* System() const final { return sys; }
        Expression* AsExpression() final { return this; }
        virtual Ast::Type* Type() const { return type; }
        virtual bool IsConstant() { return false; }
    private:
        SystemModule* sys;
        Ast::Type* type;
    };

}
