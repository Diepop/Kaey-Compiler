#include "ModuleContext.hpp"

#include "Function.hpp"
#include "Types.hpp"

namespace Kaey::Llvm
{
    template <class T, class ... Args>
    T* ModuleContext::GetOrCreateType(Args&&... args)
    {
        auto uni = std::make_unique<T>(this, std::forward<Args>(args)...);
        auto it = typeMap.find(uni->Name());
        if (it != typeMap.end())
            return (T*)it->second;
        auto ptr = uni.get();
        AddObject(move(uni));
        return ptr;
    }

    ModuleContext::ModuleContext(llvm::LLVMContext& ctx, llvm::Module* mod, llvm::DataLayout* dataLayout) :
        ctx(ctx), mod(mod), dataLayout(dataLayout),
        skipTypeInit(true),
        voidType(GetOrCreateType<VoidType>()),
        boolType(GetOrCreateType<BooleanType>()),
        charType(GetOrCreateType<CharacterType>()),
        floatType(GetOrCreateType<FloatType>(32)),
        doubleType(GetOrCreateType<FloatType>(64))
    {
        for (auto& name : OperatorNames)
            CreateObject<Function>(string(name));
        for (auto isSigned : { true, false })
        for (auto bits : { 8, 16, 32, 64, 128 })
            GetIntegerType(bits, isSigned);
        for (auto type : typeList)
            type->DeclareMethods();
        for (auto type : typeList)
            type->CreateMethods();
        skipTypeInit = false;
    }

    VoidType* ModuleContext::GetVoidType()
    {
        return voidType;
    }

    BooleanType* ModuleContext::GetBooleanType()
    {
        return boolType;
    }

    CharacterType* ModuleContext::GetCharacterType()
    {
        return charType;
    }

    IntegerType* ModuleContext::GetIntegerType(int bits, bool isSigned)
    {
        return GetOrCreateType<IntegerType>(bits, isSigned);
    }

    FloatType* ModuleContext::GetFloatType()
    {
        return floatType;
    }

    FloatType* ModuleContext::GetDoublingType()
    {
        return doubleType;
    }

    PointerType* ModuleContext::GetPointerType(Type* underlyingType)
    {
        return GetOrCreateType<PointerType>(underlyingType);
    }

    ReferenceType* ModuleContext::GetReferenceType(Type* underlyingType)
    {
        return GetOrCreateType<ReferenceType>(underlyingType);
    }

    ArrayType* ModuleContext::GetArrayType(Type* underlyingType, int64_t length)
    {
        return GetOrCreateType<ArrayType>(underlyingType, length);
    }

    TupleType* ModuleContext::GetTupleType(vector<Type*> elementTypes)
    {
        return GetOrCreateType<TupleType>(move(elementTypes));
    }

    VariantType* ModuleContext::GetVariantType(vector<Type*> elementTypes)
    {
        return GetOrCreateType<VariantType>(move(elementTypes));
    }

    FunctionPointerType* ModuleContext::GetFunctionPointerType(Type* returnType, vector<Type*> paramTypes, bool isVariadic)
    {
        return GetOrCreateType<FunctionPointerType>(returnType, move(paramTypes), isVariadic);
    }

    OptionalType* ModuleContext::GetOptionalType(Type* underlyingType)
    {
        return GetOrCreateType<OptionalType>(underlyingType);
    }

    ClassType* ModuleContext::DeclareClass(string name)
    {
        assert(FindType(name) == nullptr && "A type with this name is already defined!");
        return GetOrCreateType<ClassType>(move(name));
    }

    VariableReference* ModuleContext::DeclareVariable(string name, Type* type)
    {
        mod->getOrInsertGlobal(name, type->Instance());
        auto v = mod->getNamedGlobal(name);
        v->setLinkage(llvm::GlobalValue::ExternalLinkage);
        return CreateObject<VariableReference>(nullptr, move(name), type->ReferenceTo(), v);
    }

    FunctionOverload* ModuleContext::DeclareFunction(string name, ArrayView<ArgumentDeclaration> args, Type* returnType, bool isVariadic, bool noMangling)
    {
        assert(rn::all_of(args, [](auto& arg) { return !arg.Type->template Is<VoidType>(); }) && "Function parameter must not be void!");
        auto fn = FindFunction(name);
        if (!fn) fn = CreateObject<Function>(move(name));
        assert(fn->Name() != "Main" || fn->Overloads().empty() && "Main Function may not be overloaded!");
        auto result = CreateObject<FunctionOverload>(fn, args | to_vector, returnType, isVariadic, noMangling || fn->Name() == "Main");
        fn->AddOverload(result);
        return result;
    }

    Constant* ModuleContext::CreateStringConstant(string_view value)
    {
        auto charType = GetCharacterType();
        auto arrayType = GetArrayType(charType, (int)value.length() + 1);
        vector<Constant*> v;
        v.reserve(value.length() + 1);
        rn::transform(value, back_inserter(v), [=](char d) { return charType->CreateConstant(d); });
        v.emplace_back(charType->CreateConstant('\0'));
        return arrayType->CreateConstant(move(v));
    }

    void ModuleContext::AddObject(unique_ptr<IObject> ptr)
    {
        Dispatch(ptr.get(),
            [this](Function* fn)
            {
                assert(!functionMap.contains(fn->Name()) && "Function already mapped!");
                functionMap.emplace(fn->Name(), fn);
            },
            [this](Type* t)
            {
                assert(!typeMap.contains(t->Name()) && "Type already mapped!");
                types.emplace_back(t);
                typeList.emplace_back(t);
                typeMap.emplace(t->Name(), t);
                if (skipTypeInit || t->Is<ClassType>())
                    return;
                t->DeclareMethods();
                t->CreateMethods();
            }
        );
        values.emplace_back(move(ptr));
    }

    Type* ModuleContext::FindType(string_view name) const
    {
        auto it = typeMap.find(name);
        return it != typeMap.end() ? it->second : nullptr;
    }

    Function* ModuleContext::FindFunction(string_view name) const
    {
        auto it = functionMap.find(name);
        return it != functionMap.end() ? it->second : nullptr;
    }
}
