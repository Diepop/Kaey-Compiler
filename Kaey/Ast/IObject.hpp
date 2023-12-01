#pragma once
#include "Kaey/Ast/IdentifierList.hpp"
#include "Kaey/Utility.hpp"

namespace Kaey::Ast
{
    using std::string_view;
    using std::string;
    using std::vector;
    using std::unique_ptr;
    using std::tuple;
    using std::pair;
    using std::unordered_map;

    struct Module;
    struct SystemModule;
    struct Type;
    struct Expression;
    struct Statement;
    struct VoidType;
    struct BooleanType;
    struct CharacterType;
    struct FloatType;
    struct IntegerType;
    struct VariantType;
    struct TupleType;
    struct ArrayType;
    struct FunctionPointerType;
    struct OptionalType;
    struct PointerType;
    struct ReferenceType;
    struct VariableDeclaration;
    struct ClassDeclaration;
    struct Function;

    /**
     * \brief Base of all Ast types that need dynamic allocation.
     */
    struct IObject
    {
        virtual ~IObject() = default;
        virtual SystemModule* System() const = 0;
        virtual Type* AsType() { return nullptr; }
        virtual Expression* AsExpression() { return nullptr; }
        virtual Statement* AsStatement() { return nullptr; }
    };

    struct Context : IObject
    {
        Context(SystemModule* sys, Context* parentContext = nullptr);
        Context(Context* parentContext);

        Context(const Context&) = delete;
        Context(Context&&) = delete;

        Context& operator=(const Context&) = delete;
        Context& operator=(Context&&) = delete;

        ~Context() override = default;

        SystemModule* System() const final;

        ArrayView<IObject*> Objects() const;
        ArrayView<Type*> Types() const;
        ArrayView<Function*> Functions() const;
        ArrayView<ClassDeclaration*> Classes() const;
        ArrayView<VariableDeclaration*> Variables() const;

        ReferenceType* GetReferenceType(Type* type) const;
        PointerType* GetPointerType(Type* type) const;
        OptionalType* GetOptionalType(Type* type) const;
        FunctionPointerType* GetFunctionPointerType(vector<Type*> paramTypes, Type* returnType, bool isVariadic) const;
        ArrayType* GetArrayType(Type* underlyingType, int64_t length) const;
        TupleType* GetTupleType(vector<Type*> types) const;
        VariantType* GetVariantType(vector<Type*> types) const;
        IntegerType* GetIntegerType(int bits, bool isSigned) const;
        FloatType* GetFloatType() const;
        CharacterType* GetCharacterType() const;
        BooleanType* GetBooleanType() const;
        VoidType* GetVoidType() const;

        VariableDeclaration* FindVariable(string_view name) const;
        Type* FindType(string_view name) const;
        Function* FindFunction(string_view name) const;

        void RegisterStatement(Statement* statement);
        void RegisterType(Type* type);
        void RegisterFunction(Function* function);

        void AddObject(unique_ptr<IObject> obj) const;

        template<class T, class... Args>
        T* CreateObject(Args&&... args) const
        {
            try
            {
                if constexpr (requires { new T{ System(), std::forward<Args>(args)... }; })
                {
                    auto ptr = new T{ System(), std::forward<Args>(args)... };
                    AddObject(unique_ptr<IObject>{ptr});
                    return ptr;
                }
                else
                {
                    auto ptr = new T{ std::forward<Args>(args)... };
                    AddObject(unique_ptr<IObject>{ptr});
                    return ptr;
                }
            }
            catch (std::exception&)
            {
                return nullptr;
            }
        }

        template<class Child, class... Args>
            requires (std::is_base_of_v<Context, Child>)
        Child* CreateChild(Args&&... args)
        {
            auto ptr = CreateObject<Child>(this, std::forward<Args>(args)...);
            if constexpr (std::is_base_of_v<Statement, Child>)
                RegisterStatement(ptr);
            if constexpr (std::is_base_of_v<Type, Child>)
                RegisterType(ptr);
            if constexpr (std::is_base_of_v<Function, Child>)
                RegisterFunction(ptr);
            return ptr;
        }

    private:
        SystemModule* sys;
        Context* parentContext;
        IdentifierList<IObject> objects;
        IdentifierList<Type> types;
        IdentifierList<Function> fns;
        IdentifierList<ClassDeclaration> classes;
        IdentifierList<VariableDeclaration> vars;
    };

}
