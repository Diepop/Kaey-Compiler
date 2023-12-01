#pragma once
#include "IObject.hpp"

namespace Kaey::Llvm
{
    using std::unordered_map;
    using std::string_view;
    using std::list;

    struct Type;
    struct VariableReference;
    struct IFunctionOverload;
    struct ArgumentReference;
    struct ArgumentDeclaration;

    struct ModuleContext
    {
        ModuleContext(llvm::LLVMContext& ctx, llvm::Module* mod, llvm::DataLayout* dataLayout);

        ModuleContext(const ModuleContext&) = delete;
        ModuleContext(ModuleContext&&) = delete;
        ModuleContext& operator=(const ModuleContext&) = delete;
        ModuleContext& operator=(ModuleContext&&) = delete;

        llvm::LLVMContext& Context() const { return ctx; }
        llvm::Module* Module() const { return mod; }
        llvm::DataLayout* DataLayout() const { return dataLayout; }
        ArrayView<Type*> Types() const { return types; }
        
        VoidType* GetVoidType();
        BooleanType* GetBooleanType();
        CharacterType* GetCharacterType();
        IntegerType* GetIntegerType(int bits, bool isSigned);
        FloatType* GetFloatType();
        FloatType* GetDoublingType();
        PointerType* GetPointerType(Type* underlyingType);
        ReferenceType* GetReferenceType(Type* underlyingType);
        ArrayType* GetArrayType(Type* underlyingType, int64_t length);
        TupleType* GetTupleType(vector<Type*> elementTypes);
        VariantType* GetVariantType(vector<Type*> elementTypes);
        FunctionPointerType* GetFunctionPointerType(Type* returnType, vector<Type*> paramTypes, bool isVariadic);
        OptionalType* GetOptionalType(Type* underlyingType);

        ClassType* DeclareClass(string name);

        VariableReference* DeclareVariable(string name, Type* type);

        FunctionOverload* DeclareFunction(string name, ArrayView<ArgumentDeclaration> args, Type* returnType, bool isVariadic = false, bool noMangling = false);

        Constant* CreateStringConstant(string_view value);

        template <class T, class ... Args>
        T* CreateObject(Args&&... args)
        {
            static_assert(!std::is_base_of_v<Type, T>, "Use GetOrCreateType!");
            auto uni = std::make_unique<T>(this, std::forward<Args>(args)...);
            auto ptr = uni.get();
            AddObject(move(uni));
            return ptr;
        }

        void AddObject(unique_ptr<IObject> ptr);

        Type* FindType(string_view name) const;
        Function* FindFunction(string_view name) const;

    private:
        llvm::LLVMContext& ctx;
        llvm::Module* mod;
        llvm::DataLayout* dataLayout;
        vector<unique_ptr<IObject>> values;
        vector<Type*> types;
        unordered_map<string_view, Type*> typeMap;
        list<Type*> typeList;
        unordered_map<string_view, Function*> functionMap;

        bool skipTypeInit;
        VoidType* voidType;
        BooleanType* boolType;
        CharacterType* charType;
        FloatType* floatType;
        FloatType* doubleType;

        template<class T, class... Args>
        T* GetOrCreateType(Args&&... args);
    };

    
}
