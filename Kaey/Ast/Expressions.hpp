#pragma once
#include "Expression.hpp"
#include "Kaey/Lexer/KaeyLexer.hpp"
#include "Kaey/Ast/Types.hpp"

namespace Kaey::Ast
{
    struct UnaryExpression final : Expression::With<UnaryExpression>
    {
        UnaryExpression(SystemModule* sys, Ast::Type* type, Lexer::Token* oper, Expression* operand);
        Lexer::Token* Operator() const { return oper; }
        Expression* Operand() const { return operand; }
    private:
        Lexer::Token* oper;
        Expression* operand;
    };

    struct BinaryExpression final : Expression::With<BinaryExpression>
    {
        BinaryExpression(SystemModule* sys, Ast::Type* type, Expression* leftOperand, Lexer::Token* oper, Expression* rightOperand);
        Expression* LeftOperand() const { return leftOperand; }
        Lexer::Token* Operator() const { return oper; }
        Expression* RightOperand() const { return rightOperand; }
    private:
        Expression* leftOperand;
        Lexer::Token* oper;
        Expression* rightOperand;
    };

    struct CallExpression final : Expression::With<CallExpression>
    {
        CallExpression(SystemModule* sys, Ast::Type* type, Expression* callee, vector<Expression*> arguments);
        Expression* Callee() const { return callee; }
        ArrayView<Expression*> Arguments() const { return arguments; }
    private:
        Expression* callee;
        vector<Expression*> arguments;
    };

    struct VariableReferenceExpression final : Expression::With<VariableReferenceExpression>
    {
        VariableReferenceExpression(SystemModule* sys, VariableDeclaration* decl);
        VariableDeclaration* Declaration() const { return declaration; }
    private:
        VariableDeclaration* declaration;
    };

    struct ConstructExpression final : Expression::With<ConstructExpression>
    {
        ConstructExpression(SystemModule* sys, Ast::Type* type, vector<Expression*> arguments);
        ArrayView<Expression*> Arguments() const { return arguments; }
    private:
        vector<Expression*> arguments;
    };

    struct ExternCAttribute final : Expression::With<ExternCAttribute>
    {
        ExternCAttribute(SystemModule* sys);
    };

    struct NewExpression final : Expression::With<NewExpression>
    {
        NewExpression(SystemModule* sys, Ast::Type* type, vector<Expression*> arguments);
        ArrayView<Expression*> Arguments() const { return arguments; }
    private:
        vector<Expression*> arguments;
    };

    struct NamedExpression : Expression
    {
        NamedExpression(SystemModule* sys, Expression* arg);
        Expression* Argument() const { return arg; }
    private:
        Expression* arg;
    };

    struct DeleteExpression final : Expression::With<DeleteExpression, NamedExpression>
    {
        using ParentClass::ParentClass;
    };

    struct ReturnExpression final : Expression::With<ReturnExpression, NamedExpression>
    {
        using ParentClass::ParentClass;
    };

    struct NamedTypeExpression : Expression
    {
        NamedTypeExpression(SystemModule* sys, Ast::Type* type, Ast::Type* arg);
        Ast::Type* Argument() const { return arg; }
    private:
        Ast::Type* arg;
    };

    struct SizeOfExpression final : Expression::With<SizeOfExpression, NamedTypeExpression>
    {
        using ParentClass::ParentClass;
    };

    struct AlignOfExpression final : Expression::With<AlignOfExpression, NamedTypeExpression>
    {
        using ParentClass::ParentClass;
    };

    struct MemberAccessExpression final : Expression::With<MemberAccessExpression>
    {
        MemberAccessExpression(SystemModule* sys, Expression* owner, FieldDeclaration* field);
        Expression* Owner() const { return owner; }
        FieldDeclaration* Field() const { return field; }
    private:
        Expression* owner;
        FieldDeclaration* field;
    };

    struct SubscriptOperatorExpression final : Expression::With<SubscriptOperatorExpression>
    {
        SubscriptOperatorExpression(SystemModule* sys, Ast::Type* type, Expression* callee, vector<Expression*> arguments);
        Expression* Callee() const { return callee; }
        ArrayView<Expression*> Arguments() const { return arguments; }
    private:
        Expression* callee;
        vector<Expression*> arguments;
    };

    struct TernaryExpression final : Expression::With<TernaryExpression>
    {
        TernaryExpression(SystemModule* sys, Ast::Type* type, Expression* condition, Expression* trueExpression, Expression* falseExpression);
        Expression* Condition() const { return condition; }
        Expression* TrueExpression() const { return trueExpression; }
        Expression* FalseExpression() const { return falseExpression; }
    private:
        Expression* condition;
        Expression* trueExpression;
        Expression* falseExpression;
    };

    struct Function final : Expression::With<Function>
    {
        Function(SystemModule* sys, string name);
        FunctionType* Type() const override { return (FunctionType*)ParentClass::Type(); }
        bool IsConstant() override { return true; }
        ArrayView<FunctionDeclaration*> Overloads() const { return overloads; }
        string_view Name() { return name; }
        void AddOverload(FunctionDeclaration* fn);
        FunctionDeclaration* FindOverload(ArrayView<Ast::Type*> argTypes) const;
        vector<FunctionDeclaration*> FindCallable(ArrayView<Ast::Type*> argTypes) const;
    private:
        string name;
        vector<FunctionDeclaration*> overloads;
    };

}
