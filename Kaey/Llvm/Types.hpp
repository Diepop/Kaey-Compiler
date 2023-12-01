#pragma once
#include "Type.hpp"

namespace Kaey::Llvm
{

    struct ArrayType final : Type::With<ArrayType, Type>
    {
        ArrayType(ModuleContext* mod, Type* underlyingType, int64_t length);
        llvm::ArrayType* Instance() const override { return static_cast<llvm::ArrayType*>(Type::Instance()); }
        Type* UnderlyingType() const { return underlyingType; }
        int64_t Length() const { return length; }
        void DeclareMethods() override;
        llvm::Constant* CreateConstantValue(ArrayView<Constant*> value);
        Constant* CreateConstant(vector<Constant*> value);
    private:
        Type* underlyingType;
        int64_t length;
        unordered_map<vector<Constant*>, Constant*, RangeHasher> constantMap;
    };

    struct BooleanType final : Type::With<BooleanType, ValueParameterType>
    {
        BooleanType(ModuleContext* mod);
        void DeclareMethods() override;
        llvm::ConstantInt* CreateConstantValue(bool value);
        Constant* CreateConstant(bool value);
    private:
        Constant* trueConstant;
        Constant* falseConstant;
    };

    struct CharacterType final : IObject::With<CharacterType, ValueParameterType>
    {
        CharacterType(ModuleContext* mod);
        llvm::Constant* CreateConstantValue(char value);
        Constant* CreateConstant(char value);
        void DeclareMethods() override;
    private:
        Constant* constants[256];
    };

    struct ClassType final : IObject::With<ClassType, Type>
    {
        struct MethodOptions
        {
            bool IsVariadic = false;
            bool NoMangling = false;
        };
        ClassType(ModuleContext* mod, string name);
        llvm::StructType* Instance() const override;
        void DeclareMethods() override;
        ClassType& BeginFields();
        ClassType& DeclareField(string name, Llvm::Type* type);
        ClassType& EndFields();
        ClassType& Begin();
        FunctionOverload* DeclareConstructor(vector<ArgumentDeclaration> params);
        FunctionOverload* DeclareDestructor();
        FunctionOverload* DeclareMethod(string_view name, vector<ArgumentDeclaration> params, Llvm::Type* returnType, MethodOptions options = {});
        void End();
    private:
#ifdef _DEBUG
        bool fieldsDefined = false;
        bool defined = false;
#endif
    };

    struct FloatType final : Type::With<FloatType, ValueParameterType>
    {
        FloatType(ModuleContext* mod, int bits);
        int Bits() const { return bits; }
        void DeclareMethods() override;
        llvm::Constant* CreateConstantValue(llvm::StringRef value);
        llvm::Constant* CreateConstantValue(float value);
        llvm::Constant* CreateConstantValue(double value);
        Constant* CreateConstant(float value);
    private:
        int bits;
        unordered_map<string, Constant*> constantMap;
    };

    struct FunctionPointerType final : Type::With<FunctionPointerType, ValueParameterType>
    {
        FunctionPointerType(ModuleContext* mod, Type* returnType, vector<Type*> parameterTypes, bool isVariadic);
        ArrayView<Type*> ParameterTypes() const { return parameterTypes; }
        Type* ReturnType() const { return returnType; }
        bool IsVariadic() const { return isVariadic; }
        llvm::FunctionType* Instance() const override { return (llvm::FunctionType*)ParentClass::Instance(); }
        void DeclareMethods() override;
    private:
        Llvm::Type* returnType;
        vector<Llvm::Type*> parameterTypes;
        bool isVariadic;
    };

    struct IntegerType final : Type::With<IntegerType, ValueParameterType>
    {
        IntegerType(ModuleContext* mod, int bits, bool isSigned);
        int Bits() const { return bits; }
        bool IsSigned() const { return isSigned; }
        llvm::IntegerType* Instance() const override { return (llvm::IntegerType*)Type::Instance(); }
        void DeclareMethods() override;
        llvm::ConstantInt* CreateConstantValueUnsigned(uint64_t value);
        llvm::ConstantInt* CreateConstantValue(int64_t value);
        Constant* CreateConstantUnsigned(uint64_t value);
        Constant* CreateConstant(int64_t value);
        Expression* PassArgument(FunctionOverload* fn, Expression* expr) override;
        uint64_t GetConstantValue(Constant* c);
    private:
        int bits;
        bool isSigned;
        unordered_map<uint64_t, Constant*> constantMap;
    };

    struct OptionalType final : Type::With<OptionalType, ValueParameterType>
    {
        OptionalType(ModuleContext* mod, Type* underlyingType);
        Type* UnderlyingType() const { return underlyingType; }
        void DeclareMethods() override;
    private:
        Type* underlyingType;
    };

    struct PointerType final : Type::With<PointerType, ValueParameterType>
    {
        PointerType(ModuleContext* mod, Type* underlyingType);
        llvm::PointerType* Instance() const override;
        Llvm::Type* UnderlyingType() const { return underlyingType; }
        Llvm::Type* RemovePointer() override;
        void DeclareMethods() override;
    private:
        Llvm::Type* underlyingType;
    };

    struct ReferenceType final : Type::With<ReferenceType, Type>
    {
        ReferenceType(ModuleContext* mod, Type* underlyingType);
        llvm::PointerType* Instance() const override;
        llvm::Type* TypeOfParameter() const override { return Instance(); }
        llvm::Type* TypeOfReturn() const override { return Instance(); }
        bool IsStructReturn() const override { return false; }
        ReferenceType* ReferenceTo() override { return this; }
        Llvm::Type* UnderlyingType() const { return underlyingType; }
        Llvm::Type* RemoveReference() override;
        Expression* PassArgument(FunctionOverload* fn, Expression* expr) override;
        Expression* Allocate(FunctionOverload* fn) override;
        void DeclareMethods() override;
        void Construct(FunctionOverload* fn, Expression* inst, vector<Expression*> args) override;
    private:
        Llvm::Type* underlyingType;
    };

    struct TupleType final : Type::With<TupleType, Type>
    {
        TupleType(ModuleContext* mod, vector<Type*> elementTypes);
        ArrayView<Type*> ElementTypes() const { return elementTypes; }
        Type* ElementType(int i) const { return elementTypes[i]; }
        int ElementCount() const { return (int)elementTypes.size(); }
        void DeclareMethods() override;
        bool IsTriviallyCopyable() const override { return isTriviallyCopyable; }
    private:
        vector<Type*> elementTypes;
        FunctionOverload* copyConstructor;
        FunctionOverload* defConstructor;
        bool isTriviallyCopyable;
    };

    struct VariantType final : Type::With<VariantType, Type>
    {
        VariantType(ModuleContext* mod, vector<Type*> elementTypes);
        ArrayView<Type*> ElementTypes() const { return elementTypes; }
        Type* ElementType(int i) const { return elementTypes[i]; }
        int ElementCount() const { return (int)elementTypes.size(); }
        void DeclareMethods() override;
        bool IsTriviallyCopyable() const override { return isTriviallyCopyable; }
        Expression* GetTypeByIndex(FunctionOverload* fn, Expression* inst, int64_t i);
    private:
        vector<Type*> elementTypes;
        FunctionOverload* copyConstructor;
        FunctionOverload* defConstructor;
        bool isTriviallyCopyable;
        Type* dominantType;
    };

    struct VoidType final : IObject::With<VoidType, Type>
    {
        VoidType(ModuleContext* mod);
        llvm::Type* TypeOfParameter() const override;
        bool IsStructReturn() const override { return false; }
        Expression* Allocate(FunctionOverload*) override;
        void DeclareMethods() override;
        void Construct(FunctionOverload* fn, Expression* inst, vector<Expression*> args) override;
    };

}
