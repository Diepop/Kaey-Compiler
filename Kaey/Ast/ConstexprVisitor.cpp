#include "ConstexprVisitor.hpp"
#include "Kaey/Ast/Expressions.hpp"
#include "Kaey/Ast/Statements.hpp"
#include "Kaey/Ast/Types.hpp"

using namespace Kaey::Lexer;

namespace Kaey::Ast
{
    Expression* ConstexprVisitor::Visit(BooleanConstant* arg)
    {
        return arg;
    }

    Expression* ConstexprVisitor::Visit(CharacterConstant* arg)
    {
        return arg;
    }

    Expression* ConstexprVisitor::Visit(FloatConstant* arg)
    {
        return arg;
    }

    Expression* ConstexprVisitor::Visit(IntegerConstant* arg)
    {
        return arg;
    }

    Expression* ConstexprVisitor::Visit(VoidConstant* arg)
    {
        return arg;
    }

    Expression* ConstexprVisitor::Visit(TupleExpression* arg)
    {
        return ctx->CreateObject<TupleExpression>(arg->TypeOfTuple(), VisitRange(arg->Values(), this));
    }

    Expression* ConstexprVisitor::Visit(BinaryExpression* arg)
    {
        auto left = Dispatch(arg->LeftOperand(), this);
        auto right = Dispatch(arg->RightOperand(), this);
        if (!left || !right)
            return nullptr;
        return VariadicVisit<Expression*>({ left, right },
            [&](IntegerConstant* l, IntegerConstant* r)
            {
                auto lType = l->TypeOfInteger();
                auto rType = r->TypeOfInteger();
                assert(lType->IsSigned() == rType->IsSigned());
                auto type = lType->Bits() > rType->Bits() ? lType : rType;
                auto bType = ctx->GetBooleanType();
                return Dispatch<Expression*>(arg->Operator(),
                    [=](Plus*)   { return type->CreateConstant(l->Value() + r->Value()); },
                    [=](Minus*)  { return type->CreateConstant(l->Value() - r->Value()); },
                    [=](Star*)   { return type->CreateConstant(l->Value() * r->Value()); },
                    [=](Slash*)  { return type->CreateConstant(l->Value() / r->Value()); },
                    [=](Modulo*) { return type->CreateConstant(l->Value() % r->Value()); },

                    [=](Caret*)     { return type->CreateConstant(l->Value() ^ r->Value()); },
                    [=](Pipe*)      { return type->CreateConstant(l->Value() | r->Value()); },
                    [=](Ampersand*) { return type->CreateConstant(l->Value() & r->Value()); },

                    [=](LeftShift*)  { return type->CreateConstant(l->Value() << r->Value()); },
                    [=](RightShift*) { return type->CreateConstant(l->Value() >> r->Value()); },

                    [=](Equal*)        { return bType->CreateConstant(l->Value() == r->Value()); },
                    [=](NotEqual*)     { return bType->CreateConstant(l->Value() != r->Value()); },
                    [=](Less*)         { return bType->CreateConstant(l->Value() <  r->Value()); },
                    [=](LessEqual*)    { return bType->CreateConstant(l->Value() <= r->Value()); },
                    [=](Greater*)      { return bType->CreateConstant(l->Value() >  r->Value()); },
                    [=](GreaterEqual*) { return bType->CreateConstant(l->Value() >= r->Value()); },

                    [=](Token*) { return nullptr; }
                );
            },
            [&](BooleanConstant* l, BooleanConstant* r)
            {
                auto type = ctx->GetBooleanType();
                return Dispatch<Expression*>(arg->Operator(),
                    [=](Equal*)    { return type->CreateConstant(l->Value() == r->Value()); },
                    [=](NotEqual*) { return type->CreateConstant(l->Value() != r->Value()); },

                    [=](And*) { return type->CreateConstant(l->Value() && r->Value()); },
                    [=](Or*)  { return type->CreateConstant(l->Value() || r->Value()); },

                    [=](Token*) { return nullptr; }
                );
            },
            [&](FloatConstant* l, FloatConstant* r)
            {
                auto lt = l->TypeOfFloat();
                auto rt = r->TypeOfFloat();
                auto type = lt->Bits() > rt->Bits() ? lt : rt;
                auto bType = ctx->GetBooleanType();
                return Dispatch<Expression*>(arg->Operator(),
                    [=](Plus*)  { return type->CreateConstant(l->Value() + r->Value()); },
                    [=](Minus*) { return type->CreateConstant(l->Value() - r->Value()); },
                    [=](Star*)  { return type->CreateConstant(l->Value() * r->Value()); },
                    [=](Slash*) { return type->CreateConstant(l->Value() / r->Value()); },

                    [=](Equal*)        { return bType->CreateConstant(l->Value() == r->Value()); },
                    [=](NotEqual*)     { return bType->CreateConstant(l->Value() != r->Value()); },
                    [=](Less*)         { return bType->CreateConstant(l->Value() <  r->Value()); },
                    [=](LessEqual*)    { return bType->CreateConstant(l->Value() <= r->Value()); },
                    [=](Greater*)      { return bType->CreateConstant(l->Value() >  r->Value()); },
                    [=](GreaterEqual*) { return bType->CreateConstant(l->Value() >= r->Value()); },

                    [=](Token*) { return nullptr; }
                );
            },
            [&](Expression*, Expression*) { return nullptr; }
        );
    }

    Expression* ConstexprVisitor::Visit(UnaryExpression* arg)
    {
        auto operand = Dispatch(arg->Operand(), this);
        if (!operand)
            return nullptr;
        return Dispatch<Expression*>(operand,
            [&](IntegerConstant* v)
            {
                auto type = v->TypeOfInteger();
                return Dispatch<Expression*>(arg->Operator(),
                    [=](Plus*) { return type->CreateConstant(+v->Value()); },
                    [=](Minus*) { return type->CreateConstant(0 - v->Value()); },
                    [=](Tilde*) { return type->CreateConstant(~v->Value()); },
                    [=](Token*) { return nullptr; }
                );
            },
            [&](BooleanConstant* v)
            {
                auto type = ctx->GetBooleanType();
                return Dispatch<Expression*>(arg->Operator(),
                    [=](Exclamation*) { return type->CreateConstant(!v->Value()); },
                    [=](Token*) { return nullptr; }
                );
            },
            [&](FloatConstant* v)
            {
                auto type = v->TypeOfFloat();
                return Dispatch<Expression*>(arg->Operator(),
                    [=](Plus*) { return type->CreateConstant(+v->Value()); },
                    [=](Minus*) { return type->CreateConstant(-v->Value()); },
                    [=](Token*) { return nullptr; }
                );
            },
            [&](Expression*, Expression*) { return nullptr; }
        );
    }

    ConstexprVisitor::ConstexprVisitor(Context* ctx) : ctx(ctx)
    {

    }

}
