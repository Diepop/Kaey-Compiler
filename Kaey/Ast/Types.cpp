#include "Types.hpp"

#include "Module.hpp"
#include "Statements.hpp"

namespace Kaey::Ast
{

    ArrayType::ArrayType(SystemModule* sys, Type* underlyingType, int64_t length) :
        ParentClass(sys, "{}[{}]"_f(underlyingType->Name(), length)),
        underlyingType(underlyingType), length(length)
    {
        assert(underlyingType->IsNot<ReferenceType>());
        assert(underlyingType && length > 0);
    }

    BooleanType::BooleanType(SystemModule* mod) : ParentClass(mod, "bool"), trueConstant(new BooleanConstant(this, true)), falseConstant(new BooleanConstant(this, false))
    {
        mod->AddObject(unique_ptr<IObject>{ trueConstant });
        mod->AddObject(unique_ptr<IObject>{ falseConstant });
    }

    CharacterType::CharacterType(SystemModule* mod) : ParentClass(mod, "char")
    {

    }

    ClassType::ClassType(ClassDeclaration* declaration, string name) : ParentClass(declaration->System(), move(name)), declaration(declaration)
    {

    }

    Context* ClassType::GetContext() const
    {
        return declaration;
    }

    FloatType::FloatType(SystemModule* mod, int bits) : ParentClass(mod, "f{}"_f(bits)), bits(bits)
    {
        assert(bits == 64 || bits == 32);
    }

    FunctionPointerType::FunctionPointerType(SystemModule* mod, vector<Type*> parameterTypes, Type* returnType, bool isVariadic) :
        ParentClass(mod, "({}{})->{}"_f(join(parameterTypes | name_of_range, ", "), isVariadic ? (parameterTypes.empty() ? "..." : ", ...") : "", returnType->Name())),
        parameterTypes(move(parameterTypes)), returnType(returnType), isVariadic(isVariadic)
    {

    }

    FunctionType::FunctionType(SystemModule* sys, Function* ownerFunction, string_view name) : ParentClass(sys, "{}$type"_f(name)), ownerFunction(ownerFunction)
    {

    }

    IntegerType::IntegerType(SystemModule* mod, int bits, bool isSigned) : ParentClass(mod, "{}{}"_f(isSigned ? 'i' : 'u', bits)), bits(bits), isSigned(isSigned)
    {

    }

    OptionalType::OptionalType(SystemModule* mod, Type* underlyingType) : ParentClass(mod, "{}?"_f(underlyingType->Name())), underlyingType(underlyingType)
    {

    }

    PointerType::PointerType(SystemModule* mod, Type* underlyingType) : ParentClass(mod, "{}*"_f(underlyingType->Name())), underlyingType(underlyingType)
    {

    }

    ReferenceType::ReferenceType(SystemModule* mod, Type* underlyingType) : ParentClass(mod, "{}&"_f(underlyingType->Name())), underlyingType(underlyingType)
    {
        assert(!underlyingType->Is<ReferenceType>());
    }

    TupleType::TupleType(SystemModule* mod, vector<Type*> elementTypes) :
        ParentClass(mod, "({})"_f(join(elementTypes | name_of_range, ", "))),
        elementTypes(move(elementTypes))
    {
        assert(this->elementTypes.size() > 1);
        auto i = 0;
        for (auto ty : this->elementTypes)
            AddField(GetContext()->CreateChild<FieldDeclaration>("{}"_f(i++), ty, nullptr));
    }

    VariantType::VariantType(SystemModule* sys, vector<Type*> underlyingTypes) :
        ParentClass(sys, "({})"_f(join(underlyingTypes | name_of_range, " | "))),
        underlyingTypes(move(underlyingTypes))
    {

    }

    VoidType::VoidType(SystemModule* mod) : ParentClass(mod, "void"), constant(new VoidConstant{ this })
    {
        mod->AddObject(unique_ptr<IObject>{ constant });
    }

    ArrayExpression* ArrayType::CreateExpression(vector<Expression*> values)
    {
        assert(int64_t(values.size()) == length);
        if (rn::all_of(values, &Expression::IsConstant))
        {
            auto [it, emplaced] = constants.try_emplace(values);
            if (emplaced)
                it->second = System()->CreateObject<ArrayExpression>(this, move(values));
            return it->second;
        }
        return System()->CreateObject<ArrayExpression>(this, move(values));
    }

    ReferenceExpression::ReferenceExpression(SystemModule* sys, ReferenceType* type) : Expression(sys, type)
    {
        
    }

    ReferenceType* ReferenceExpression::Type() const
    {
        return static_cast<ReferenceType*>(Expression::Type());
    }

    ArrayExpression::ArrayExpression(ArrayType* typeOfArray, vector<Expression*> values) :
        ParentClass(typeOfArray->System(), typeOfArray->ReferenceTo()), typeOfArray(typeOfArray), values(move(values)),
        isConstant(rn::all_of(this->values, mem_fn(&Expression::IsConstant)))
    {

    }

    BooleanConstant* BooleanType::CreateConstant(bool value)
    {
        return value ? trueConstant : falseConstant;
    }

    BooleanConstant::BooleanConstant(BooleanType* boolType, bool value) :
        ParentClass(boolType->System(), boolType->ReferenceTo()), boolType(boolType), value(value)
    {

    }

    CharacterConstant* CharacterType::CreateConstant(char value)
    {
        auto [it, emplaced] = constants.try_emplace(value);
        if (emplaced)
            it->second = System()->CreateObject<CharacterConstant>(this, value);
        return it->second;
    }

    CharacterConstant::CharacterConstant(CharacterType* typeOfChar, char value) :
        ParentClass(typeOfChar->System(), typeOfChar->ReferenceTo()),
        value(value), typeOfChar(typeOfChar)
    {

    }

    FloatConstant::FloatConstant(FloatType* typeOfFloat, double value) :
        ParentClass(typeOfFloat->System(), typeOfFloat->ReferenceTo()),
        typeOfFloat(typeOfFloat), value(value)
    {
        
    }

    FloatConstant* FloatType::CreateConstant(double value)
    {
        auto [it, emplaced] = constants.try_emplace(value);
        if (emplaced)
            it->second = System()->CreateObject<FloatConstant>(this, value);
        return it->second;
    }

    IntegerConstant* IntegerType::CreateConstant(uint64_t value)
    {
        auto [it, emplaced] = constants.try_emplace(value);
        if (emplaced)
            it->second = System()->CreateObject<IntegerConstant>(this, value);
        return it->second;
    }

    IntegerConstant::IntegerConstant(IntegerType* typeOfInteger, uint64_t value) :
        ParentClass(typeOfInteger->System(), typeOfInteger->ReferenceTo()),
        value(value), typeOfInteger(typeOfInteger)
    {

    }

    TupleExpression* TupleType::CreateExpression(vector<Expression*> value)
    {
        assert(value.size() == ElementTypes().size());
        auto [it, emplaced] = constants.try_emplace(value);
        if (emplaced)
            it->second = System()->CreateObject<TupleExpression>(this, move(value));
        return it->second;
    }

    TupleExpression::TupleExpression(SystemModule* mod, TupleType* typeOfTuple, vector<Expression*> values) :
        ParentClass(mod, typeOfTuple->ReferenceTo()),
        typeOfTuple(typeOfTuple), values(move(values)),
        isConstant(rn::all_of(this->values, mem_fn(&Expression::IsConstant)))
    {

    }

    VoidConstant::VoidConstant(VoidType* type) : ParentClass(type->System(), type->ReferenceTo())
    {

    }

}
