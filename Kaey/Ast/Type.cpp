#include "Type.hpp"

#include "Module.hpp"
#include "Expressions.hpp"
#include "Statements.hpp"
#include "Types.hpp"

namespace Kaey::Ast
{

    Type::Type(SystemModule* sys, string name) : sys(sys), name(move(name)), defaultConstructor(nullptr)
    {

    }

    string_view Type::Name() const
    {
        return name;
    }

    ArrayView<FieldDeclaration*> Type::Fields() const
    {
        return fields;
    }

    ArrayView<MethodDeclaration*> Type::Methods() const
    {
        return methods;
    }

    ArrayView<ConstructorDeclaration*> Type::Constructors() const
    {
        return constructors;
    }

    ConstructorDeclaration* Type::DefaultConstructor() const
    {
        return defaultConstructor;
    }

    PointerType* Type::PointerTo()
    {
        return System()->GetPointerType(this);
    }

    ReferenceType* Type::ReferenceTo()
    {
        return System()->GetReferenceType(this);
    }

    OptionalType* Type::OptionalTo()
    {
        return System()->GetOptionalType(this);
    }

    Type* Type::Decay()
    {
        return this;
    }

    Type* Type::RemoveReference()
    {
        return this;
    }

    Type* Type::RemovePointer()
    {
        return this;
    }

    void Type::AddField(FieldDeclaration* field)
    {
        fields.Add(field);
    }

    void Type::AddMethod(MethodDeclaration* method)
    {
        methods.emplace_back(method);
    }

    void Type::AddConstructor(ConstructorDeclaration* constructor)
    {
        assert(constructor->ClassType() == this);
        if (constructor->IsDefault())
        {
            assert(defaultConstructor == nullptr);
            defaultConstructor = constructor;
        }
        constructors.emplace_back(constructor);
    }

    FieldDeclaration* Type::DeclareField(string name, Type* type, Expression* initializeExpression)
    {
        auto field = GetContext()->CreateChild<FieldDeclaration>(move(name), type, initializeExpression);
        AddField(field);
        return field;
    }

    MethodDeclaration* Type::DeclareMethod(string name, vector<pair<string, Type*>> params, Type* returnType)
    {
        auto check = rn::contains(OperatorNames, name);
        auto fn = check ? System()->FindFunction(name) : FindMethod(name);
        if (check && !fns.Find(name))
            fns.Add(fn);
        if (!fn) fn = AddMethod(move(name));
        auto method = GetContext()->CreateChild<MethodDeclaration>(this, fn, move(params), returnType);
        AddMethod(method);
        return method;
    }

    ConstructorDeclaration* Type::DeclareConstructor(vector<pair<string, Type*>> params, bool isDefault, bool isDestructor)
    {
        auto c = GetContext()->CreateChild<ConstructorDeclaration>(this, move(params), isDefault, isDestructor);
        AddConstructor(c);
        return c;
    }

    Function* Type::FindMethod(string_view name) const
    {
        return fns.Find(name);
    }

    Function* Type::AddMethod(string name)
    {
        assert(!FindMethod(name));
        auto fn = System()->CreateObject<Function>(move(name));
        fns.Add(fn);
        return fn;
    }

    void Type::CreateDefaultFunctions()
    {
        DeclareMethod(AMPERSAND_OPERATOR, {  }, PointerTo());
    }

    FieldDeclaration* Type::FindField(string_view name) const
    {
        return fields.Find(name);
    }

    Context* Type::GetContext() const
    {
        if (!ctx)
            ctx = std::make_unique<Context>(System(), System());
        return ctx.get();
    }

}
