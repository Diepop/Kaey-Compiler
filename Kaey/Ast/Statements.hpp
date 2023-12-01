#pragma once
#include "Kaey/Ast/IObject.hpp"
#include "Kaey/Ast/Expression.hpp"
#include "Kaey/Ast/Statement.hpp"
#include "Kaey/Ast/Type.hpp"

namespace Kaey::Ast
{
    struct Function;

    struct VariableDeclaration : Statement::With<VariableDeclaration>
    {
        VariableDeclaration(SystemModule* sys, Context* parentContext, string name, Ast::Type* type, Expression* initializingExpression = nullptr);
        string_view Name() const { return name; }
        Ast::Type* Type() const { return type; }
        Expression* InitializingExpression() const { return initializingExpression; }
        VariableReferenceExpression* Reference() const { return reference; }
        void AddInitializingExpression(Expression* expr);
    private:
        string name;
        Ast::Type* type;
        Expression* initializingExpression;
        VariableReferenceExpression* reference;
    };

    struct TupleVariableDeclaration : Statement::With<TupleVariableDeclaration>
    {
        TupleVariableDeclaration(SystemModule* sys, Context* parentContext, vector<VariableDeclaration*> variables, TupleType* type, Expression* initializeExpression);
        ArrayView<VariableDeclaration*> Variables() const { return variables; }
        TupleType* Type() const { return type; }
        Expression* InitializeExpression() const { return initializeExpression; }
    private:
        vector<VariableDeclaration*> variables;
        TupleType* type;
        Expression* initializeExpression;
    };

    struct FunctionDeclaration : Statement::With<FunctionDeclaration>
    {
        FunctionDeclaration(SystemModule* sys, Context* parentContext, Function* ownerFunction, vector<pair<string, Ast::Type*>> parameters, Ast::Type* returnType, bool isVariadic);
        string_view Name() const;
        ParameterDeclaration* Parameter(int i) const { return parameters[i]; }
        ArrayView<ParameterDeclaration*> Parameters() const { return parameters; }
        Ast::Type* ReturnType() const { return returnType; }
        bool IsVariadic() const { return isVariadic; }
        ArrayView<Statement*> Statements() const { return statements; }
        FunctionPointerType* Type() const { return type; }
        virtual bool IsMethod() const { return false; }
        virtual bool IsConstructor() const { return false; }
        virtual bool IsDestructor() const { return false; }
        void AddStatement(Statement* statement);
    private:
        Function* ownerFunction;
        vector<ParameterDeclaration*> parameters;
        Ast::Type* returnType;
        bool isVariadic;
        FunctionPointerType* type;
        vector<Statement*> statements;
    };

    struct MethodDeclaration : Statement::With<MethodDeclaration, FunctionDeclaration>
    {
        MethodDeclaration(SystemModule* sys, Context* parentContext, Ast::Type* classType, Function* ownerFunction, vector<pair<string, Ast::Type*>> parameters, Ast::Type* returnType);
        Ast::Type* ClassType() const { return classType; }
        bool IsMethod() const override { return true; }
    private:
        Ast::Type* classType;
    };

    struct ConstructorDeclaration final : Statement::With<ConstructorDeclaration, MethodDeclaration>
    {
        ConstructorDeclaration(SystemModule* sys, Context* parentContext, Ast::Type* classType, vector<pair<string, Ast::Type*>> parameters, bool isDefault, bool isDestructor);
        bool IsDefault() const { return isDefault; }
        bool IsConstructor() const override { return !isDestructor; }
        bool IsDestructor() const override { return isDestructor; }
        ArrayView<FieldInitializerStatement*> FieldInitialiers() const { return fieldInitialiers; }
        ArrayView<DelegateInitializerStatement*> DelegateInitialiers() const { return delegateInitialiers; }
        FieldInitializerStatement* AddFieldInitializer(FieldDeclaration* field, vector<Expression*> args);
        DelegateInitializerStatement* AddDelegateInitializer(Ast::Type* base, vector<Expression*> args);
    private:
        bool isDefault;
        bool isDestructor;
        vector<FieldInitializerStatement*> fieldInitialiers;
        vector<DelegateInitializerStatement*> delegateInitialiers;
    };

    struct WhileStatement final : Statement::With<WhileStatement>
    {
        WhileStatement(SystemModule* sys, Context* parentContext);
        Expression* Condition;
        Statement* LoopStatement;
    };

    struct ForStatement final : Statement::With<ForStatement>
    {
        ForStatement(SystemModule* sys, Context* parentContext);
        Statement* StartStatement;
        Expression* Condition;
        Expression* Increment;
        Statement* LoopStatement;
    };

    struct SwitchStatement final : Statement::With<SwitchStatement>
    {
        SwitchStatement(SystemModule* sys, Context* parentContext);
        Expression* Condition;
        vector<pair<Expression*, Statement*>> Cases;
    };

    struct ParameterDeclaration final : Statement::With<ParameterDeclaration, VariableDeclaration>
    {
        using ParentClass::ParentClass;
    };

    struct FieldDeclaration final : Statement::With<FieldDeclaration, VariableDeclaration>
    {
        using ParentClass::ParentClass;
    };

    struct ClassDeclaration final : Statement::With<ClassDeclaration>
    {
        ClassDeclaration(Context* parentContext, string name);
        ClassType* DeclaredType() const { return declaredType; }
        string_view Name() const;
    private:
        ClassType* declaredType;
    };

    struct BlockStatement final : Statement::With<BlockStatement>
    {
        BlockStatement(SystemModule* sys, Context* parentContext);
        vector<Statement*> Statements;
    };

    struct ExpressionStatement : Statement::With<ExpressionStatement>
    {
        ExpressionStatement(SystemModule* sys, Context* parentContext);
        Expression* UnderlyingExpression;
    };

    struct AttributeDeclaration final : Statement::With<AttributeDeclaration>
    {
        AttributeDeclaration(SystemModule* sys, Context* parentContext, vector<Expression*> expressions);
        ArrayView<Expression*> Expressions() const { return expressions; }
    private:
        vector<Expression*> expressions;
    };

    struct DelegateInitializerStatement final : Statement::With<DelegateInitializerStatement>
    {
        DelegateInitializerStatement(SystemModule* sys, Context* parentContext, Ast::Type* base, vector<Expression*> args);
        Ast::Type* Base() const { return base; }
        ArrayView<Expression*> Arguments() const { return args; }
    private:
        Ast::Type* base;
        vector<Expression*> args;
    };

    struct FieldInitializerStatement final : Statement::With<FieldInitializerStatement>
    {
        FieldInitializerStatement(SystemModule* sys, Context* parentContext, FieldDeclaration* field, vector<Expression*> args);
        FieldDeclaration* Field() const { return field; }
        ArrayView<Expression*> Arguments() const { return args; }
    private:
        FieldDeclaration* field;
        vector<Expression*> args;
    };

    struct IfStatement final : Statement::With<IfStatement>
    {
        IfStatement(SystemModule* sys, Context* parentContext);
        Expression* Condition;
        Statement* TrueStatement;
        Statement* FalseStatement;
    };


}
