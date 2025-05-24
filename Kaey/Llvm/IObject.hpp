#pragma once
#include "Kaey/Utility.hpp"

namespace Kaey::Llvm
{
    using std::string;
    using std::vector;
    using std::unique_ptr;

    struct ModuleContext;

    struct IObject : Variant<IObject,
        struct FunctionOverload,
        struct IntrinsicFunction,
        struct Expression,
        struct VariableReference,
        struct Constant,
        struct VoidType,
        struct BooleanType,
        struct CharacterType,
        struct IntegerType,
        struct FloatType,
        struct PointerType,
        struct ReferenceType,
        struct ArrayType,
        struct TupleType,
        struct FunctionPointerType,
        struct VariantType,
        struct ClassType,
        struct Function,
        struct DeletedFunctionOverload,
        struct OptionalType
    >
    {
        virtual ~IObject() = default;
        virtual ModuleContext* Module() const = 0;
    };

}
