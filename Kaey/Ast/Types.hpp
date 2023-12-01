#pragma once
#include "Kaey/Ast/Expression.hpp"
#include "Kaey/Ast/Type.hpp"

namespace Kaey::Ast
{
    struct ArrayExpression;
    struct BooleanConstant;
    struct CharacterConstant;
    struct ClassDeclaration;
    struct FloatConstant;
    struct IntegerConstant;
    struct TupleExpression;
    struct VariantExpression;
    struct VoidConstant;

    struct ArrayType final : Type::With<ArrayType>
    {
        ArrayType(SystemModule* sys, Type* underlyingType, int64_t length);
        Type* UnderlyingType() const { return underlyingType; }
        int64_t Length() const { return length; }
        ArrayExpression* CreateExpression(vector<Expression*> values);
    private:
        Type* underlyingType;
        int64_t length;
        unordered_map<ArrayView<Expression*>, ArrayExpression*, RangeHasher> constants;
    };

    struct BooleanType final : Type::With<BooleanType>
    {
        BooleanType(SystemModule* mod);
        BooleanConstant* CreateConstant(bool value);
    private:
        BooleanConstant* trueConstant, * falseConstant;
    };

    struct CharacterType final : Type::With<CharacterType>
    {
        CharacterType(SystemModule* mod);
        CharacterConstant* CreateConstant(char value);
    private:
        unordered_map<uint64_t, CharacterConstant> constants;
    };

    struct ClassType final : Type::With<ClassType>
    {
        ClassType(ClassDeclaration* declaration, string name);
        ClassDeclaration* Declaration() const { return declaration; }
        Context* GetContext() const override;
    private:
        ClassDeclaration* declaration;
    };

    struct FloatType final : Type::With<FloatType>
    {
        FloatType(SystemModule* mod);
        FloatConstant* CreateConstant(float value);
    private:
        unordered_map<float, FloatConstant*> constants;
    };

    struct FunctionPointerType final : Type::With<FunctionPointerType>
    {
        FunctionPointerType(SystemModule* mod, vector<Type*> parameterTypes, Type* returnType, bool isVariadic);
        ArrayView<Type*> ParameterTypes() const { return parameterTypes; }
        Type* ReturnType() const { return returnType; }
        bool IsVariadic() const { return isVariadic; }
    private:
        vector<Type*> parameterTypes;
        Type* returnType;
        bool isVariadic;
    };

    struct FunctionType final : Type::With<FunctionType>
    {
        FunctionType(SystemModule* sys, Function* ownerFunction, string_view name);
        Function* OwnerFunction() const { return ownerFunction; }
    private:
        Function* ownerFunction;
    };

    struct IntegerType final : Type::With<IntegerType>
    {
        IntegerType(SystemModule* mod, int bits, bool isSigned);
        int Bits() const { return bits; }
        bool IsSigned() const { return isSigned; }
        IntegerConstant* CreateConstant(uint64_t value);
    private:
        int bits;
        bool isSigned;
        unordered_map<uint64_t, IntegerConstant*> constants;
    };

    struct OptionalType final : Type::With<OptionalType>
    {
        OptionalType(SystemModule* mod, Type* underlyingType);
        Type* Decay() override { return UnderlyingType(); }
        Type* UnderlyingType() const { return underlyingType; }
    private:
        Type* underlyingType;
    };

    struct PointerType final : Type::With<PointerType>
    {
        PointerType(SystemModule* mod, Type* underlyingType);
        Type* Decay() override { return UnderlyingType(); }
        Type* RemovePointer() override { return UnderlyingType(); }
        Type* UnderlyingType() const { return underlyingType; }
    private:
        Type* underlyingType;
    };

    struct ReferenceType final : Type::With<ReferenceType>
    {
        ReferenceType(SystemModule* mod, Type* underlyingType);
        Type* Decay() override { return UnderlyingType(); }
        Type* RemoveReference() override { return UnderlyingType(); }
        Type* UnderlyingType() const { return underlyingType; }
    private:
        Type* underlyingType;
    };

    struct TupleType final : Type::With<TupleType>
    {
        TupleType(SystemModule* mod, vector<Type*> elementTypes);
        ArrayView<Type*> ElementTypes() const { return elementTypes; }
        TupleExpression* CreateExpression(vector<Expression*> value);
    private:
        vector<Type*> elementTypes;
        unordered_map<ArrayView<Expression*>, TupleExpression*, RangeHasher> constants;
    };

    struct VariantType final : Type::With<VariantType>
    {
        VariantType(SystemModule* sys, vector<Type*> underlyingTypes);
        ArrayView<Type*> UnderlyingTypes() const { return underlyingTypes; }
        Type* UnderlyingType(size_t i) const { return underlyingTypes[i]; }
    private:
        vector<Type*> underlyingTypes;
    };

    struct VoidType final : Type::With<VoidType>
    {
        VoidType(SystemModule* mod);
        VoidConstant* CreateConstant() const { return constant; }
    private:
        VoidConstant* constant;
    };

    struct ReferenceExpression : Expression
    {
        ReferenceExpression(SystemModule* sys, ReferenceType* type);
        ReferenceType* Type() const final;
    };

    struct ArrayExpression final : Expression::With<ArrayExpression, ReferenceExpression>
    {
        ArrayExpression(ArrayType* typeOfArray, vector<Expression*> values);
        ArrayType* TypeOfArray() const { return typeOfArray; }
        ArrayView<Expression*> Values() const { return values; }
        bool IsConstant() override { return isConstant; }
    private:
        ArrayType* typeOfArray;
        vector<Expression*> values;
        bool isConstant;
    };

    struct BooleanConstant final : Expression::With<BooleanConstant, ReferenceExpression>
    {
        BooleanConstant(BooleanType* boolType, bool value);
        bool Value() const { return value; }
        BooleanType* TypeOfBoolean() const { return boolType; }
        bool IsConstant() override { return true; }
    private:
        BooleanType* boolType;
        bool value;
    };

    struct CharacterConstant final : Expression::With<CharacterConstant, ReferenceExpression>
    {
        CharacterConstant(CharacterType* typeOfChar, char value);
        char Value() const { return value; }
        CharacterType* TypeOfChar() const { return typeOfChar; }
        bool IsConstant() override { return true; }
    private:
        char value;
        CharacterType* typeOfChar;
    };

    struct FloatConstant final : Expression::With<FloatConstant, ReferenceExpression>
    {
        FloatConstant(FloatType* typeOfFloat, float value);
        float Value() const { return value; }
        FloatType* TypeOfFloat() const { return typeOfFloat; }
        bool IsConstant() override { return true; }
    private:
        float value;
        FloatType* typeOfFloat;
    };

    struct IntegerConstant final : Expression::With<IntegerConstant, ReferenceExpression>
    {
        IntegerConstant(IntegerType* typeOfInteger, uint64_t value);
        IntegerType* TypeOfInteger() const { return typeOfInteger; }
        uint64_t Value() const { return value; }
        bool IsConstant() override { return true; }
    private:
        uint64_t value;
        IntegerType* typeOfInteger;
    };

    struct TupleExpression final : Expression::With<TupleExpression, ReferenceExpression>
    {
        TupleExpression(SystemModule* mod, TupleType* typeOfTuple, vector<Expression*> values);
        ArrayView<Expression*> Values() const { return values; }
        TupleType* TypeOfTuple() const { return typeOfTuple; }
        bool IsConstant() override { return isConstant; }
    private:
        TupleType* typeOfTuple;
        vector<Expression*> values;
        bool isConstant;
    };

    struct VoidConstant final : Expression::With<VoidConstant>
    {
        VoidConstant(VoidType* type);
        bool IsConstant() override { return true; }
    };

}
