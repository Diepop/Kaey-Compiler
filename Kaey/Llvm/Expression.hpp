#pragma once
#include "IObject.hpp"

namespace Kaey::Llvm
{
    using std::string_view;

    struct Type;

    struct Expression : IObject::With<Expression>
    {
        Expression(ModuleContext* mod, Llvm::Type* type, llvm::Value* value);
        ModuleContext* Module() const override { return mod; }
        virtual Llvm::Type* Type() const { return type; }
        virtual llvm::Value* Value() const { return value; }
        Expression* GetField(FunctionOverload* caller, string_view name);
        Expression* GetField(FunctionOverload* caller, int index);
    private:
        ModuleContext* mod;
        Llvm::Type* type;
        llvm::Value* value;
    };

    struct Constant final : IObject::With<Constant, Expression>
    {
        Constant(ModuleContext* mod, ReferenceType* type, llvm::GlobalValue* value, llvm::Constant* constant);
        llvm::GlobalValue* Value() const override { return static_cast<llvm::GlobalValue*>(Expression::Value()); }
        llvm::Constant* ConstantValue() const { return constant; }
    private:
        llvm::Constant* constant;
    };

}
