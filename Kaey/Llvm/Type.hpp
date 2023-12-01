#pragma once
#include "Kaey/Llvm/IObject.hpp"

namespace Kaey::Llvm
{
    using std::string;
    using std::string_view;
    using std::pair;
    using std::unordered_map;

    struct Function;
    struct FunctionOverload;
    struct IFunctionOverload;
    struct ArgumentReference;
    struct ArgumentDeclaration;
    struct ReferenceType;
    struct PointerType;
    struct Constant;
    struct Type;

    struct FieldDeclaration
    {
        string Name;
        Type* Type;
        int Index;
    };

    struct Type : IObject
    {
        Type(ModuleContext* mod, string name, llvm::Type* type);
        ModuleContext* Module() const final { return mod; }
        virtual llvm::Type* Instance() const { return type; }
        ArrayView<Function*> Functions() const { return functions; }
        llvm::Value* AllocateValue(FunctionOverload* fn);
        virtual Expression* Allocate(FunctionOverload* fn);
        virtual void Construct(FunctionOverload* caller, Expression* inst, vector<Expression*> args);
        virtual void Destruct(FunctionOverload* caller, Expression* inst);
        
        virtual ArgumentReference* CaptureArgument(FunctionOverload* function, const ArgumentDeclaration& declaration, llvm::Argument* arg);
        virtual Expression* PassArgument(FunctionOverload* fn, Expression* expr);
        virtual llvm::Type* TypeOfParameter() const;
        virtual llvm::Type* TypeOfReturn() const;
        virtual bool IsStructReturn() const;
        virtual ReferenceType* ReferenceTo();
        virtual PointerType* PointerTo();
        virtual OptionalType* OptionalTo();
        virtual Type* RemoveReference();
        virtual Type* RemovePointer();
        virtual bool IsTriviallyCopyable() const;
        int64_t Alignment() const;
        int64_t Size() const;
        string_view Name() const { return name; }
        Function* AddFunction(string name);
        Function* FindFunction(string_view name);
        const FieldDeclaration* FindField(string_view name);
        virtual Expression* GetField(FunctionOverload* caller, Expression* inst, int index);
        IFunctionOverload* FindMethodOverload(string_view name, vector<Type*> paramTypes);
        ArrayView<FieldDeclaration*> Fields() const { return fields; }
        virtual void DeclareMethods();
        virtual void CreateMethods();
    protected:
        void AddField(string name, Type* type);
        struct FunctionDeclarationArgs
        {
            vector<ArgumentDeclaration> Arguments;
            Type* ReturnType = nullptr;
            bool IsVariadic = false;
            bool NoMangling = false;
        };
        FunctionOverload* AddFunctionDeclaration(Function* fn, FunctionDeclarationArgs args, std::function<void(FunctionOverload*)> declFn);
    private:
        ModuleContext* mod;
        string name;
        llvm::Type* type;
        vector<Function*> functions;
        unordered_map<string, Function*> functionMap;
        vector<FieldDeclaration*> fields;
        unordered_map<string_view, FieldDeclaration*> fieldMap;
        vector<unique_ptr<FieldDeclaration>> fieldInsts;
        vector<std::function<void()>> createFns;
        Function* AddFunctionUnchecked(string name);
    };

    struct ValueParameterType : Type
    {
        using Type::Type;
        ArgumentReference* CaptureArgument(FunctionOverload* fn, const ArgumentDeclaration& declaration, llvm::Argument* arg) override;
        Expression* PassArgument(FunctionOverload* fn, Expression* expr) override;
        llvm::Type* TypeOfParameter() const override;
        llvm::Type* TypeOfReturn() const override;
        bool IsStructReturn() const override;
    };

}
