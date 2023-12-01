#pragma once
#include "Kaey/Ast/IObject.hpp"
#include "Kaey/Ast/Statement.hpp"

namespace Kaey::Ast
{
    struct Function;

    struct Type : IObject, Variant<Type,
        struct ArrayType,
        struct BooleanType,
        struct CharacterType,
        struct ClassType,
        struct FloatType,
        //struct DoubleType,
        struct FunctionPointerType,
        struct FunctionType,
        struct IntegerType,
        struct OptionalType,
        struct PointerType,
        struct ReferenceType,
        struct TupleType,
        struct VariantType,
        struct VoidType
    >
    {
        Type(SystemModule* sys, string name);
        Type(const Type&) = delete;
        Type(Type&&) = delete;
        Type& operator=(const Type&) = delete;
        Type& operator=(Type&&) = delete;
        ~Type() override = default;
        string_view Name() const;
        ArrayView<FieldDeclaration*> Fields() const;
        ArrayView<MethodDeclaration*> Methods() const;
        ArrayView<ConstructorDeclaration*> Constructors() const;
        ConstructorDeclaration* DefaultConstructor() const;
        PointerType* PointerTo();
        ReferenceType* ReferenceTo();
        OptionalType* OptionalTo();
        virtual Type* Decay();
        virtual Type* RemoveReference();
        virtual Type* RemovePointer();
        virtual void AddField(FieldDeclaration* field);
        virtual void AddMethod(MethodDeclaration* method);
        virtual void AddConstructor(ConstructorDeclaration* constructor);
        FieldDeclaration* DeclareField(string name, Type* type, Expression* initializeExpression = nullptr);
        MethodDeclaration* DeclareMethod(string name, vector<pair<string, Type*>> params, Type* returnType);
        ConstructorDeclaration* DeclareConstructor(vector<pair<string, Type*>> params, bool isDefault = false, bool isDestructor = false);
        Function* FindMethod(string_view name) const;
        Function* AddMethod(string name);
        void CreateDefaultFunctions();
        FieldDeclaration* FindField(string_view name) const;
        Type* AsType() override { return this; }
        SystemModule* System() const override { return sys; }
        virtual Context* GetContext() const;
    private:
        SystemModule* sys;
        string name;
        ConstructorDeclaration* defaultConstructor;
        IdentifierList<FieldDeclaration> fields;
        vector<MethodDeclaration*> methods;
        vector<ConstructorDeclaration*> constructors;
        IdentifierList<Function> fns;
        mutable unique_ptr<Context> ctx;
    };

}
