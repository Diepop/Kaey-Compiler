#pragma once
#include "Kaey/Ast/Module.hpp"
#include "Kaey/Ast/ConstexprVisitor.hpp"
#include "Kaey/Parser/KaeyParser.hpp"

namespace Kaey::Ast
{
    struct ExpressionVisitor;

    struct TypeVisitor : Parser::BaseType::Visitor<Type*>
    {
        Type* Visit(Parser::DecoratedType* arg);

        Type* Visit(Parser::TupleType* arg);

        Type* Visit(Parser::IdentifierType* arg);

        Type* Visit(Parser::ArrayType* arg);

        Type* Visit(Parser::VariantType* arg);

        TypeVisitor(Context* ctx, ExpressionVisitor* expressionVisitor);

    private:
        Context* ctx;
        ConstexprVisitor constexprEvaluator;
        ExpressionVisitor* expressionVisitor;
    };

    struct ExpressionVisitor : Parser::Expression::Visitor<Expression*>
    {
        Expression* Visit(Parser::IntegerExpression* arg);

        Expression* Visit(Parser::BooleanExpression* arg);

        Expression* Visit(Parser::FloatingExpression* arg);

        Expression* Visit(Parser::StringLiteralExpression* arg);

        Expression* Visit(Parser::TupleExpression* arg);

        Expression* Visit(Parser::ParenthisedExpression* arg);

        Expression* Visit(Parser::BinaryOperatorExpression* arg);

        Expression* Visit(Parser::UnaryOperatorExpression* arg);

        Expression* Visit(Parser::IdentifierExpression* arg);

        Expression* Visit(Parser::CallExpression* arg);

        Expression* Visit(Parser::TernaryExpression* arg);

        Expression* Visit(Parser::NewExpression* arg);

        Expression* Visit(Parser::NamedOperatorExpression* arg);

        Expression* Visit(Parser::NamedTypeOperatorExpression* arg);

        Expression* Visit(Parser::SubscriptOperator* arg);

        Expression* Visit(Parser::MemberAccessExpression* arg);

        explicit ExpressionVisitor(Context* ctx);

    private:
        Context* ctx;
        TypeVisitor typeVisitor;

    };

    struct StatementVisitor : Parser::Statement::Visitor<Statement*>, ExpressionVisitor, TypeVisitor
    {
        using ExpressionVisitor::Visit;
        using TypeVisitor::Visit;

        Statement* Visit(Parser::VariableDeclaration* arg);

        Statement* Visit(Parser::TupleVariableDeclaration* arg);

        Statement* Visit(Parser::BlockStatement* arg);

        Statement* Visit(Parser::IfStatement* arg);

        Statement* Visit(Parser::ExpressionStatement* arg);

        Statement* Visit(Parser::WhileStatement* arg);

        Statement* Visit(Parser::ForStatement* arg);

        Statement* Visit(Parser::SwitchStatement* arg);

        explicit StatementVisitor(Context* ctx);

    private:
        Context* ctx;
    };

    struct ClassVisitor final : StatementVisitor
    {
        using StatementVisitor::Visit;

        Statement* Visit(Parser::FunctionDeclaration* arg);

        Statement* Visit(Parser::PropertyDeclaration* arg);

        Statement* Visit(Parser::ClassDeclaration* arg);

        explicit ClassVisitor(ClassDeclaration* decl);

    private:
        ClassDeclaration* decl;

    };

    struct ModuleVisitor final : ExpressionVisitor, TypeVisitor, Parser::Statement::Visitor<void>
    {
        using ExpressionVisitor::Visit;
        using TypeVisitor::Visit;

        void Visit(Parser::FunctionDeclaration* arg);

        void Visit(Parser::ClassDeclaration* arg);

        void Visit(Parser::VariableDeclaration* arg);

        void Visit(Parser::Module* parseTree);

        explicit ModuleVisitor(Module* mod);

    private:
        Module* mod;
        unordered_map<Parser::FunctionDeclaration*, FunctionDeclaration*> fnMap;
    };

    Module* CreateAst(SystemModule* sys, Parser::Module* parseTree);

}
