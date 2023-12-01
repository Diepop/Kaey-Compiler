#include "Expressions.hpp"

#include "Module.hpp"
#include "Statements.hpp"
#include "Types.hpp"

namespace Kaey::Ast
{

    UnaryExpression::UnaryExpression(SystemModule* sys, Ast::Type* type, Lexer::Token* oper, Expression* operand) :
        ParentClass(sys, type), oper(oper),
        operand(operand)
    {
        
    }

    BinaryExpression::BinaryExpression(SystemModule* sys, Ast::Type* type, Expression* leftOperand, Lexer::Token* oper, Expression* rightOperand) :
        ParentClass(sys, type), leftOperand(leftOperand),
        oper(oper), rightOperand(rightOperand)
    {
        
    }

    CallExpression::CallExpression(SystemModule* sys, Ast::Type* type, Expression* callee, vector<Expression*> arguments) :
        ParentClass(sys, type), callee(callee), arguments(move(arguments))
    {
        
    }

    VariableReferenceExpression::VariableReferenceExpression(SystemModule* sys, VariableDeclaration* decl) :
        ParentClass(sys, decl->Type()->ReferenceTo()), declaration(decl)
    {
        
    }

    ConstructExpression::ConstructExpression(SystemModule* sys, Ast::Type* type, vector<Expression*> arguments) :
        ParentClass(sys, type), arguments(move(arguments))
    {
        
    }

    ExternCAttribute::ExternCAttribute(SystemModule* sys) : ParentClass(sys, nullptr)
    {
        
    }

    NewExpression::NewExpression(SystemModule* sys, Ast::Type* type, vector<Expression*> arguments) : ParentClass(sys, type), arguments(move(arguments))
    {

    }

    NamedExpression::NamedExpression(SystemModule* sys, Expression* arg) : Expression(sys, sys->GetVoidType()), arg(arg)
    {

    }

    MemberAccessExpression::MemberAccessExpression(SystemModule* sys, Expression* owner, FieldDeclaration* field) :
        ParentClass(sys, field->Type()->ReferenceTo()), owner(owner), field(field)
    {

    }

    SubscriptOperatorExpression::SubscriptOperatorExpression(SystemModule* sys, Ast::Type* type, Expression* callee, vector<Expression*> arguments) :
        ParentClass(sys, type), callee(callee), arguments(move(arguments))
    {

    }

    TernaryExpression::TernaryExpression(SystemModule* sys, Ast::Type* type, Expression* condition, Expression* trueExpression, Expression* falseExpression) :
        ParentClass(sys, type), condition(condition), trueExpression(trueExpression), falseExpression(falseExpression)
    {

    }

    NamedTypeExpression::NamedTypeExpression(SystemModule* sys, Ast::Type* type, Ast::Type* arg) :
        Expression(sys, type->ReferenceTo()), arg(arg)
    {

    }

    Function::Function(SystemModule* sys, string name) : ParentClass(sys, sys->CreateObject<FunctionType>(this, name)->ReferenceTo()), name(move(name))
    {

    }

    void Function::AddOverload(FunctionDeclaration* fn)
    {
        overloads.emplace_back(fn);
    }

    FunctionDeclaration* Function::FindOverload(ArrayView<Ast::Type*> argTypes) const
    {
        for (auto overload : overloads) if (overload->Type()->ParameterTypes() == argTypes)
            return overload;
        return nullptr;
    }

    vector<FunctionDeclaration*> Function::FindCallable(ArrayView<Ast::Type*> argTypes) const
    {
        if (auto f = FindOverload(argTypes))
            return { f };
        vector<FunctionDeclaration*> result;
        for (auto overload : overloads)
        {
            auto ty = overload->Type();
            auto paramTypes = ty->ParameterTypes();
            if (argTypes.size() < paramTypes.size())
                continue;
            if (argTypes.size() > paramTypes.size() &&
                !ty->IsVariadic())
                continue;
            if (!rn::equal(paramTypes, argTypes | vs::take(paramTypes.size()), [](Ast::Type* l, Ast::Type* r)
                {
                    return l == r || l == r->RemoveReference();
                }))
                continue;
                result.emplace_back(overload);
        }
        return result;
    }

}
