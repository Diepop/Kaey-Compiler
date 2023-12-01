#include "Statements.hpp"

#include "Module.hpp"
#include "Expressions.hpp"
#include "Types.hpp"

namespace Kaey::Ast
{
    VariableDeclaration::VariableDeclaration(SystemModule* sys, Context* parentContext, string name, Ast::Type* type, Expression* initializingExpression) :
        ParentClass(sys, parentContext), name(move(name)),
        type(type), initializingExpression(initializingExpression),
        reference(sys->CreateObject<VariableReferenceExpression>(this))
    {

    }

    void VariableDeclaration::AddInitializingExpression(Expression* expr)
    {
        assert(!InitializingExpression() && "Variable is already initialized!");
        initializingExpression = expr;
    }

    FunctionDeclaration::FunctionDeclaration(SystemModule* sys, Context* parentContext, Function* ownerFunction, vector<pair<string, Ast::Type*>> parameters, Ast::Type* returnType, bool isVariadic) :
        ParentClass(sys, parentContext),
        ownerFunction(ownerFunction), parameters(parameters | vs::transform([&](auto& p) { return CreateChild<ParameterDeclaration>(move(p.first), p.second); }) | to_vector),
        returnType(returnType ? returnType : sys->GetVoidType()), isVariadic(isVariadic),
        type(sys->GetFunctionPointerType(Parameters() | type_of_range | to_vector, this->returnType, isVariadic))
    {
        ownerFunction->AddOverload(this);
    }

    string_view FunctionDeclaration::Name() const
    {
        return ownerFunction->Name();
    }

    void FunctionDeclaration::AddStatement(Statement* statement)
    {
        assert(statement);
        statements.emplace_back(statement);
    }

    MethodDeclaration::MethodDeclaration(SystemModule* sys, Context* parentContext, Ast::Type* classType, Function* ownerFunction, vector<pair<string, Ast::Type*>> parameters, Ast::Type* returnType) :
        ParentClass(sys, parentContext, ownerFunction, [&]
        {
            parameters.emplace(parameters.begin(), "this", classType->ReferenceTo());
            return move(parameters);
        }(), returnType, false),
        classType(classType)
    {
        
    }

        ConstructorDeclaration::ConstructorDeclaration(SystemModule* sys, Context* parentContext, Ast::Type* classType, vector<pair<string, Ast::Type*>> parameters, bool isDefault, bool isDestructor) :
        ParentClass(sys, parentContext, classType, [&]
        {
            assert(!isDestructor || !isDefault);
            auto fn = classType->FindMethod(CONSTRUCTOR);
            if (!fn) fn = classType->AddMethod(CONSTRUCTOR);
            return fn;
        }(), move(parameters), nullptr),
        isDefault(isDefault),
        isDestructor(isDestructor)
    {

    }

    FieldInitializerStatement* ConstructorDeclaration::AddFieldInitializer(FieldDeclaration* field, vector<Expression*> args)
    {
        return fieldInitialiers.emplace_back(CreateChild<FieldInitializerStatement>(field, move(args)));
    }

    DelegateInitializerStatement* ConstructorDeclaration::AddDelegateInitializer(Ast::Type* base, vector<Expression*> args)
    {
        return delegateInitialiers.emplace_back(CreateChild<DelegateInitializerStatement>(base, move(args)));
    }

    ClassDeclaration::ClassDeclaration(Context* parentContext, string name) : ParentClass(parentContext), declaredType(System()->CreateObject<ClassType>(this, move(name)))
    {
        
    }

    string_view ClassDeclaration::Name() const
    {
        return DeclaredType()->Name();
    }

    ExpressionStatement::ExpressionStatement(SystemModule* sys, Context* parentContext) : ParentClass(sys, parentContext), UnderlyingExpression(nullptr)
    {
        
    }

    AttributeDeclaration::AttributeDeclaration(SystemModule* sys, Context* parentContext, vector<Expression*> expressions) : ParentClass(sys, parentContext), expressions(move(expressions))
    {
        
    }

    IfStatement::IfStatement(SystemModule* sys, Context* parentContext) :
        ParentClass(sys, parentContext), Condition(nullptr), TrueStatement(nullptr), FalseStatement(nullptr)
    {

    }

    BlockStatement::BlockStatement(SystemModule* sys, Context* parentContext) : ParentClass(sys, parentContext)
    {

    }

    FieldInitializerStatement::FieldInitializerStatement(SystemModule* sys, Context* parentContext, FieldDeclaration* field, vector<Expression*> args) : ParentClass(sys, parentContext), field(field), args(move(args))
    {

    }

    DelegateInitializerStatement::DelegateInitializerStatement(SystemModule* sys, Context* parentContext, Ast::Type* base, vector<Expression*> args) : ParentClass(sys, parentContext), base(base), args(move(args))
    {

    }

    WhileStatement::WhileStatement(SystemModule* sys, Context* parentContext) : ParentClass(sys, parentContext), Condition(nullptr), LoopStatement(nullptr)
    {

    }

    ForStatement::ForStatement(SystemModule* sys, Context* parentContext) : ParentClass(sys, parentContext), StartStatement(nullptr), Condition(nullptr), Increment(nullptr), LoopStatement(nullptr)
    {

    }

    TupleVariableDeclaration::TupleVariableDeclaration(SystemModule* sys, Context* parentContext, vector<VariableDeclaration*> variables, TupleType* type, Expression* initializeExpression) :
        ParentClass(sys, parentContext), variables(move(variables)), type(type), initializeExpression(initializeExpression)
    {

    }

    SwitchStatement::SwitchStatement(SystemModule* sys, Context* parentContext) : ParentClass(sys, parentContext), Condition(nullptr)
    {

    }

}
