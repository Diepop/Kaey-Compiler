#include "Module.hpp"

#include "Expressions.hpp"
#include "Statements.hpp"
#include "Types.hpp"

namespace Kaey::Ast
{
    Context::Context(SystemModule* sys, Context* parentContext) : sys(sys), parentContext(parentContext != this ? parentContext : nullptr)
    {

    }

    Context::Context(Context* parentContext) : Context(parentContext->System(), parentContext)
    {
        
    }

    SystemModule* Context::System() const
    {
        return sys;
    }

    ArrayView<IObject*> Context::Objects() const
    {
        return objects;
    }

    ArrayView<Type*> Context::Types() const
    {
        return types;
    }

    ArrayView<Function*> Context::Functions() const
    {
        return fns;
    }

    ArrayView<ClassDeclaration*> Context::Classes() const
    {
        return classes;
    }

    ArrayView<VariableDeclaration*> Context::Variables() const
    {
        return vars;
    }

    ReferenceType* Context::GetReferenceType(Type* type) const
    {
        return System()->GetReferenceType(type);
    }

    PointerType* Context::GetPointerType(Type* type) const
    {
        return System()->GetPointerType(type);
    }

    OptionalType* Context::GetOptionalType(Type* type) const
    {
        return System()->GetOptionalType(type);
    }

    FunctionPointerType* Context::GetFunctionPointerType(vector<Type*> paramTypes, Type* returnType, bool isVariadic) const
    {
        return System()->GetFunctionPointerType(paramTypes, returnType, isVariadic);
    }

    ArrayType* Context::GetArrayType(Type* underlyingType, int64_t length) const
    {
        return System()->GetArrayType(underlyingType, length);
    }

    TupleType* Context::GetTupleType(vector<Type*> types) const
    {
        return System()->GetTupleType(move(types));
    }

    VariantType* Context::GetVariantType(vector<Type*> types) const
    {
        return System()->GetVariantType(move(types));
    }

    IntegerType* Context::GetIntegerType(int bits, bool isSigned) const
    {
        return System()->GetIntegerType(bits, isSigned);
    }

    FloatType* Context::GetFloatType() const
    {
        return System()->GetFloatType();
    }

    CharacterType* Context::GetCharacterType() const
    {
        return System()->GetCharacterType();
    }

    BooleanType* Context::GetBooleanType() const
    {
        return System()->GetBooleanType();
    }

    VoidType* Context::GetVoidType() const
    {
        return System()->GetVoidType();
    }

    VariableDeclaration* Context::FindVariable(string_view name) const
    {
        auto v = vars.Find(name);
        if (!v && parentContext)
            return parentContext->FindVariable(name);
        return v;
    }

    Type* Context::FindType(string_view name) const
    {
        auto ty = types.Find(name);
        if (!ty && parentContext)
            return parentContext->FindType(name);
        return ty;
    }

    Function* Context::FindFunction(string_view name) const
    {
        auto fn = fns.Find(name);
        if (!fn && parentContext)
            return parentContext->FindFunction(name);
        return fn;
    }

    void Context::RegisterStatement(Statement* statement)
    {
        Dispatch(statement,
            [&](VariableDeclaration* var)
            {
                vars.Add(var);
            },
            [&](ClassDeclaration* decl)
            {
                RegisterType(decl->DeclaredType());
            }
        );
    }

    void Context::RegisterType(Type* type)
    {
        assert(!types.Find(type->Name()));
        types.Add(type);
        if (auto cd = type->As<ClassType>())
            classes.Add(cd->Declaration());
        if (type->IsNot<VoidType, ReferenceType>())
            type->DeclareMethod(AMPERSAND_OPERATOR, {  }, type->PointerTo());
    }

    void Context::RegisterFunction(Function* function)
    {
        assert(!FindFunction(function->Name()));
        fns.Add(function);
    }

    void Context::AddObject(unique_ptr<IObject> obj) const
    {
        System()->AddObject(move(obj));
    }

    Module::Module(SystemModule* sys, string name) : Context(sys, sys), name(move(name))
    {
        assert(this == System() || Name() != SystemModule::DefaultName && "System module name is reserved!");
        if (this != System())
            ImportModule(System());
    }

    ArrayView<Module*> Module::ImportedModules() const
    {
        return importedModules;
    }

    string_view Module::Name() const
    {
        return name;
    }

    void Module::ImportModule(Module* mod)
    {
        auto& mods = importedModules;
        if (rn::find(mods, mod) != mods.end())
            return;
        mods.emplace_back(mod);
    }

    FunctionDeclaration* Module::DeclareFunction(string name, vector<pair<string, Type*>> parameters, Type* returnType, bool isVariadic)
    {
        auto f = FindFunction(name);
        if (!f)
        {
            f = System()->CreateObject<Function>(move(name));
            RegisterFunction(f);
        }
        return CreateChild<FunctionDeclaration>(f, move(parameters), returnType, isVariadic);
    }

    VariableDeclaration* Module::DeclareVariable(string name, Type* type, Expression* initializeExpression)
    {
        return CreateChild<VariableDeclaration>(move(name), type, initializeExpression);
    }

    ClassDeclaration* Module::DeclareClass(string name)
    {
        assert(!FindType(name));
        return CreateChild<ClassDeclaration>(move(name));
    }

    const string SystemModule::DefaultName = "System";

    SystemModule::SystemModule() : Module(this, SystemModule::DefaultName)
    {
        VoidTy  = CreateObject<VoidType>();
        BoolTy  = CreateObject<BooleanType>();
        CharTy  = CreateObject<CharacterType>();
        UInt8   = CreateObject<IntegerType>(8,  false);
        UInt16  = CreateObject<IntegerType>(16, false);
        UInt32  = CreateObject<IntegerType>(32, false);
        UInt64  = CreateObject<IntegerType>(64, false);
        Int8    = CreateObject<IntegerType>(8,  true);
        Int16   = CreateObject<IntegerType>(16, true);
        Int32   = CreateObject<IntegerType>(32, true);
        Int64   = CreateObject<IntegerType>(64, true);
        FloatTy = CreateObject<FloatType>();

        for (auto& n : OperatorNames)
            RegisterFunction(CreateObject<Function>(string(n)));

        RegisterType(VoidTy);
        RegisterType(BoolTy);
        RegisterType(CharTy);

        RegisterType(UInt8);
        RegisterType(UInt16);
        RegisterType(UInt32);
        RegisterType(UInt64);

        RegisterType(Int8);
        RegisterType(Int16);
        RegisterType(Int32);
        RegisterType(Int64);

        RegisterType(FloatTy);

        static constexpr auto& n1 = "_1";
        static constexpr auto& n2 = "_2";

        for (auto ty : IntTypes)
        {
            auto ref = ty->ReferenceTo();
            ty->DeclareMethod(PLUS_OPERATOR, {  }, ref);
            ty->DeclareMethod(MINUS_OPERATOR, {  }, ref);

            ty->DeclareMethod(INCREMENT_OPERATOR, {  }, ref);
            ty->DeclareMethod(DECREMENT_OPERATOR, {  }, ref);

            ty->DeclareMethod(PLUS_OPERATOR, { { "_1", ref } }, ref);
            ty->DeclareMethod(MINUS_OPERATOR, { { "_1", ref } }, ref);
            ty->DeclareMethod(STAR_OPERATOR, { { "_1", ref } }, ref);
            ty->DeclareMethod(SLASH_OPERATOR, { { "_1", ref } }, ref);
            ty->DeclareMethod(MODULO_OPERATOR, { { "_1", ref } }, ref);

            ty->DeclareMethod(LESS_OPERATOR, { { "_1", ty } }, BoolTy);
            ty->DeclareMethod(GREATER_OPERATOR, { { "_1", ty } }, BoolTy);
            ty->DeclareMethod(LESS_EQUAL_OPERATOR, { { "_1", ty } }, BoolTy);
            ty->DeclareMethod(GREATER_EQUAL_OPERATOR, { { "_1", ty } }, BoolTy);

            ty->CreateDefaultFunctions();
        }

        for (auto ty : Types()) if (ty->IsNot<VoidType, ReferenceType>())
            ty->DeclareMethod(AMPERSAND_OPERATOR, {  }, ty->PointerTo());

    }

    ReferenceType* SystemModule::GetReferenceType(Type* type)
    {
        if (auto ref = type->As<ReferenceType>())
            return ref;
        auto& map = ReferenceMap;
        auto it = map.find(type);
        if (it == map.end())
        {
            bool _;
            tie(it, _) = map.emplace(type, CreateObject<ReferenceType>(type));
        }
        return it->second;
    }

    PointerType* SystemModule::GetPointerType(Type* type)
    {
        assert(type->IsNot<ReferenceType>() && type->IsNot<FunctionPointerType>());
        auto& map = PointerMap;
        auto it = map.find(type);
        if (it == map.end())
        {
            bool _;
            tie(it, _) = map.emplace(type, CreateObject<PointerType>(type));
            auto ptrTy = it->second;
            auto ty = ptrTy->UnderlyingType();
            if (ty->IsNot<VoidType>())
            {
                auto ref = ty->ReferenceTo();
                ptrTy->DeclareMethod(STAR_OPERATOR, {  }, ref);
                for (auto iTy : IntTypes)
                    ptrTy->DeclareMethod(SUBSCRIPT_OPERATOR, { { "i", iTy } }, ref);
            }
        }
        return it->second;
    }

    OptionalType* SystemModule::GetOptionalType(Type* type)
    {
        auto& map = OptionalMap;
        auto it = map.find(type);
        if (it == map.end())
        {
            bool _;
            tie(it, _) = map.emplace(type, CreateObject<OptionalType>(type));
            auto ty = it->second;
            ty->DeclareMethod(NULL_COALESCING_OPERATOR, { { "v", ty->UnderlyingType() } }, ty->UnderlyingType());
            ty->DeclareMethod(AMPERSAND_OPERATOR, {  }, ty->PointerTo());
        }
        return it->second;
    }

    FunctionPointerType* SystemModule::GetFunctionPointerType(vector<Type*> paramTypes, Type* returnType, bool isVariadic)
    {
        assert(rn::none_of(paramTypes, [](Type* ptr) { return ptr->Is<VoidType>(); }));
        auto it = FunctionPointerMap.find({ paramTypes, returnType, isVariadic });
        return it != FunctionPointerMap.end() ? it->second : CreateObject<FunctionPointerType>(move(paramTypes), returnType ? returnType : VoidTy, isVariadic);
    }

    ArrayType* SystemModule::GetArrayType(Type* underlyingType, int64_t length)
    {
        assert(length > 0);
        auto it = ArrayMap.find({ underlyingType, length });
        return it != ArrayMap.end() ? it->second : CreateObject<ArrayType>(underlyingType, length);
    }

    TupleType* SystemModule::GetTupleType(vector<Type*> types)
    {
        assert(!types.empty());
        auto it = TupleMap.find(types);
        return it != TupleMap.end() ? it->second : CreateObject<TupleType>(move(types));
    }

    VariantType* SystemModule::GetVariantType(vector<Type*> types)
    {
        assert(!types.empty());
        auto it = VariantMap.find(types);
        return it != VariantMap.end() ? it->second : CreateObject<VariantType>(move(types));
    }

    IntegerType* SystemModule::GetIntegerType(int bits, bool isSigned)
    {
        assert(bits == 64 || bits == 32 || bits == 16 || bits == 8);
        switch (bits)
        {
        case 8:  return isSigned ? Int8  : UInt8;
        case 16: return isSigned ? Int16 : UInt16;
        case 32: return isSigned ? Int32 : UInt32;
        case 64: return isSigned ? Int64 : UInt64;
        default: throw std::runtime_error("Invalid Arguments!");
        }
    }

    VoidType* SystemModule::GetVoidType()
    {
        return VoidTy;
    }

    BooleanType* SystemModule::GetBooleanType()
    {
        return BoolTy;
    }

    CharacterType* SystemModule::GetCharacterType()
    {
        return CharTy;
    }

    FloatType* SystemModule::GetFloatType()
    {
        return FloatTy;
    }

    void SystemModule::AddObject(unique_ptr<IObject> obj)
    {
        objects.emplace_back(move(obj));
    }

}
