#include "Expression.hpp"

#include "Types.hpp"
#include "Function.hpp"

namespace Kaey::Llvm
{
    Expression::Expression(ModuleContext* mod, Llvm::Type* type, llvm::Value* value):
        mod(mod),
        type(type),
        value(value)
    {

    }

    Expression* Expression::GetField(FunctionOverload* caller, string_view name)
    {
        assert(Type()->Is<ReferenceType>() && "Expression must be an instance!");
        Expression* expr = this;
        auto type = Type()->As<ReferenceType>()->UnderlyingType();
        auto field = type->FindField(name);
        if (!field && type->Is<PointerType>())
        {
            type = type->RemovePointer();
            field = type->FindField(name);
            caller->BeginCall(STAR_OPERATOR);
                caller->AddCallParameter(this);
            expr = caller->EndCall();
        }
        assert(field && "Field not found!");
        return type->GetField(caller, expr, field->Index);
    }

    Expression* Expression::GetField(FunctionOverload* caller, int index)
    {
        assert(Type()->Is<ReferenceType>() && "Expression must be an instance!");
        auto type = Type()->As<ReferenceType>()->UnderlyingType();
        return type->GetField(caller, this, index);
    }

    Constant::Constant(ModuleContext* mod, ReferenceType* type, llvm::GlobalValue* value, llvm::Constant* constant) : ParentClass(mod, type, value), constant(constant)
    {

    }
}
