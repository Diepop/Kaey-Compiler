#pragma once
#include "IObject.hpp"
#include "Statement.hpp"
#include "Expression.hpp"
#include "Type.hpp"

namespace Kaey::Ast
{
    struct Module : Context
    {
        Module(SystemModule* sys, string name);

        Module(const Module&) = delete;
        Module(Module&&) = delete;

        Module& operator=(const Module&) = delete;
        Module& operator=(Module&&) = delete;

        ~Module() override = default;

        string_view Name() const;

        ArrayView<Module*> ImportedModules() const;

        void ImportModule(Module* mod);

        FunctionDeclaration* DeclareFunction(string name, vector<pair<string, Type*>> parameters, Type* returnType, bool isVariadic);

        VariableDeclaration* DeclareVariable(string name, Type* type, Expression* initializeExpression = nullptr);

        ClassDeclaration* DeclareClass(string name);

    private:
        string name;
        vector<Module*> importedModules;
    };

    struct SystemModule final : Module
    {
        static const string DefaultName;

        SystemModule();

        SystemModule(const SystemModule&) = delete;
        SystemModule(SystemModule&&) = delete;

        SystemModule& operator=(const SystemModule&) = delete;
        SystemModule& operator=(SystemModule&&) = delete;

        ~SystemModule() override = default;

        ReferenceType* GetReferenceType(Type* type);
        PointerType* GetPointerType(Type* underlyingType);
        OptionalType* GetOptionalType(Type* type);
        FunctionPointerType* GetFunctionPointerType(vector<Type*> paramTypes, Type* returnType, bool isVariadic);
        ArrayType* GetArrayType(Type* underlyingType, int64_t length);
        TupleType* GetTupleType(vector<Type*> types);
        VariantType* GetVariantType(vector<Type*> types);
        IntegerType* GetIntegerType(int bits, bool isSigned);
        VoidType* GetVoidType();
        BooleanType* GetBooleanType();
        CharacterType* GetCharacterType();
        FloatType* GetFloatType(int bits);

        void AddObject(unique_ptr<IObject> obj);

    private:
        vector<unique_ptr<IObject>> objects;
        VoidType* VoidTy;
        BooleanType* BoolTy;
        CharacterType* CharTy;
        union
        {
            IntegerType* IntTypes[8];
            struct
            {
                IntegerType* SignedTypes[4];
                IntegerType* UnsignedTypes[4];
            };
            struct
            {
                IntegerType* Int8, * Int16, * Int32, * Int64;
                IntegerType* UInt8, * UInt16, * UInt32, * UInt64;
            };
        };

        union
        {
            FloatType* FloatTypes[2];
            struct
            {
                FloatType* Float32;
                FloatType* Float64;
            };
        };
        
        unordered_map<Type*, ReferenceType*> ReferenceMap;
        unordered_map<Type*, PointerType*> PointerMap;
        unordered_map<Type*, OptionalType*> OptionalMap;
        unordered_map<tuple<ArrayView<Type*>, Type*, bool>, FunctionPointerType*, TupleHasher> FunctionPointerMap;
        unordered_map<pair<Type*, int64_t>, ArrayType*, TupleHasher> ArrayMap;
        unordered_map<ArrayView<Type*>, TupleType*, RangeHasher> TupleMap;
        unordered_map<ArrayView<Type*>, VariantType*, RangeHasher> VariantMap;
    };

}
