#pragma once
#include "Expression.hpp"
#include "Types.hpp"

namespace Kaey::Llvm
{
    using std::stack;
    using std::unordered_map;
    using std::string;
    using std::string_view;

    using IRBuilder = llvm::IRBuilder<>;

    struct ReturnCall {  };

    struct IFunctionOverload;
    struct FunctionOverload;
    struct Type;
    struct Function;

    struct ArgumentDeclaration
    {
        string Name;
        Type* Type = nullptr;
        Constant* Initializer = nullptr;
    };

    struct IFunctionOverload : IObject
    {
        virtual Function* Owner() const = 0;
        virtual FunctionPointerType* FunctionType() const = 0;
        virtual Llvm::Type* ReturnType() const = 0;
        virtual ArrayView<ArgumentDeclaration> ParameterDeclarations() const = 0;
        virtual ArrayView<Llvm::Type*> ParameterTypes() const = 0;
        virtual bool IsVariadic() const = 0;
        virtual bool IsImplicit() const { return true; }
        virtual bool IsEmpty() const { return false; }
        virtual Expression* Call(FunctionOverload* caller, ArrayView<Expression*> args, Expression* resultInst) = 0;
    };

    struct VariableReference : IObject::With<VariableReference, Expression>
    {
        VariableReference(ModuleContext* mod, FunctionOverload* function, string name, ReferenceType* type, llvm::Value* value);
        FunctionOverload* Function() const { return function; }
        string_view Name() const { return name; }
    private:
        FunctionOverload* function;
        string name;
    };

    struct ArgumentReference final : VariableReference
    {
        using VariableReference::VariableReference;
    };

    struct FunctionOverload final : IObject::With<FunctionOverload, IFunctionOverload>
    {
        using Callee = std::variant<Function*, IFunctionOverload*, Llvm::Type*>;
        using CallOptions = std::variant<Expression*, string, ReturnCall>;

        struct BlockContext
        {
            std::unordered_multimap<string_view, VariableReference*> Variables;
            vector<Expression*> Instances;
            bool Returned = false;
            bool Returning = false;
        };

        struct CallContext
        {
            Callee Callee;
            vector<Expression*> Arguments;
            CallOptions Options;
        };

        struct IfContext
        {
            Llvm::Type* retType;
            llvm::Value* Condition;
            llvm::BasicBlock* TrueBranch;
            llvm::BasicBlock* FalseBranch;
            llvm::BasicBlock* ElseBranch;
        };

        struct WhileContext
        {
            llvm::Value* Condition;
            llvm::BasicBlock* CheckBranch;
            llvm::BasicBlock* TrueBranch;
            llvm::BasicBlock* FalseBranch;
        };

        struct SwitchContext
        {
            llvm::Value* Condition;
            llvm::SwitchInst* SwitchInst;
            vector<pair<llvm::ConstantInt*, llvm::BasicBlock*>> Cases;
            llvm::BasicBlock* DefBlock;
            llvm::BasicBlock* EndBlock;
        };

        FunctionOverload(ModuleContext* mod, Function* owner, vector<ArgumentDeclaration> paramDecl, Llvm::Type* returnType, bool isVariadic, bool noMangling);
        ModuleContext* Module() const override { return mod; }
        Function* Owner() const override { return owner; }
        FunctionPointerType* FunctionType() const override { return functionType; }
        Llvm::Type* ReturnType() const override;
        ArrayView<Llvm::Type*> ParameterTypes() const override;
        bool IsVariadic() const override;
        llvm::FunctionType* Type() const { return type; }
        llvm::Function* Value() const { return function; }
        ArrayView<ArgumentDeclaration> ParameterDeclarations() const override { return parameterDeclarations; }
        ArrayView<ArgumentReference*> Parameters() const { return parameters; }
        ArgumentReference* Parameter(int i) const { return parameters[i]; }

        template<size_t N>
        auto UnpackParameters() const
        {
            return UnpackParametersImpl(std::make_index_sequence<N>{});
        }

        Expression* Call(FunctionOverload* caller, ArrayView<Expression*> args, Expression* resultInst) override;

        /// <summary>
        /// Begins a new scope.
        /// </summary>
        void Begin();

        /// <summary>
        /// Ends the last scope created.
        /// </summary>
        void End();

        IRBuilder* EntryBuilder() { return entryBuilder.get(); }
        IRBuilder* InitBuilder() { return initBuilder.get(); }
        IRBuilder* Builder() { return builder.get(); }

        void CreateReturn(Expression* expression);

        void BeginReturn();
        void EndReturn();

        ///<summary>
        /// Add and option to a function call.
        ///</summary>
        ///<remarks>
        /// Must be used before "BeginCall".
        ///</remarks>
        void AddOption(CallOptions option);

        void BeginCall(Callee callee);
        void BeginCall(string_view callee);
        void AddCallParameter(Expression* expr);
        Expression* EndCall();

        Expression* AddExpression(Expression* expr);

        /// <summary>
        /// Calls a function without any arguments and return the value.
        /// </summary>
        /// <remarks>
        /// Same effect as:
        /// BeginCall(...);
        /// EndCall();
        /// </remarks>
        Expression* CallUnscoped(Callee callee);

        void BeginIf(Llvm::Type* retType = nullptr);
        void AddIfCondition(Expression* cond);
        void BeginElse();
        void EndIf();

        void BeginWhile();
        void AddWhileCondition(Expression* cond);
        void EndWhile();

        void BeginSwitch();
        void AddSwitchCondition(Expression* cond);
        void BeginCase();
        void AddCaseValue(Constant* v);
        void EndCase();
        void EndSwitch();

        /// <summary>
        /// Allocates system memory based on a defined type.
        /// </summary>
        /// <param name="type"></param>
        /// <returns></returns>
        Expression* Malloc(Llvm::Type* type);

        /// <summary>
        /// Frees allocated system memory.
        /// </summary>
        /// <param name="ptr"></param>
        /// <returns></returns>
        Expression* Free(Expression* ptr);

        Expression* GetField(Expression* owner, int index);

        Expression* GetField(Expression* owner, auto index) { return GetField(owner, (int)index); }

        /// <summary>
        /// Cast a pointer or reference to another type, equivalent to reinterpret_cast.
        /// </summary>
        /// <param name="ptr">A pointer or reference to a value.</param>
        /// <param name="type">The destination type. Should be the underlying type!</param>
        /// <returns></returns>
        Expression* PointerCast(Expression* ptr, Llvm::Type* type);

        void Construct(Expression* inst, vector<Expression*> args);

        void Destruct(Expression* inst);

        Expression* CreateAddressOf(Expression* inst);

    private:
        Function* owner;
        ModuleContext* mod;
        FunctionPointerType* functionType;
        llvm::FunctionType* type;
        llvm::Function* function;
        vector<ArgumentDeclaration> parameterDeclarations;
        //Code generation
        llvm::BasicBlock* entryBlock;
        llvm::BasicBlock* initBlock;
        unique_ptr<IRBuilder> entryBuilder;
        unique_ptr<IRBuilder> initBuilder;
        unique_ptr<IRBuilder> builder;
        vector<ArgumentReference*> parameters;
        Expression* structReturn;
        stack<CallContext> callStack;
        vector<BlockContext> scopeStack;
        CallOptions nextOption;
        stack<IfContext> ifStack;
        stack<WhileContext> whileStack;
        stack<SwitchContext> switchStack;
#ifdef _DEBUG
        int varCount = 0;
        bool ended = false;
#endif
        BlockContext& CurrentContext();
        void AddVariable(VariableReference* v, ptrdiff_t offset = 0);
        void DestructScopeVariables();
        VariableReference* AllocateVariable(string name, Llvm::Type* type, int offset = 0);

        template<size_t... Is>
        auto UnpackParametersImpl(std::index_sequence<Is...>) const
        {
            if constexpr (sizeof...(Is) == 2)
                return std::make_pair(Parameter(0), Parameter(1));
            else return std::make_tuple(Parameter(Is)...);
        }
    };

    struct IntrinsicFunction final : IObject::With<IntrinsicFunction, IFunctionOverload>
    {
        using Callback = std::function<Expression*(FunctionOverload*, ArrayView<Expression*>, Expression*)>;
        static const Callback Empty;
        IntrinsicFunction(ModuleContext* mod, Function* owner, FunctionPointerType* type, const Callback& fn);
        Function* Owner() const override { return owner; }
        ModuleContext* Module() const override { return mod; }
        FunctionPointerType* FunctionType() const override { return type; }
        Llvm::Type* ReturnType() const override;
        ArrayView<ArgumentDeclaration> ParameterDeclarations() const override { return paramenterDeclarations; }
        ArrayView<Llvm::Type*> ParameterTypes() const override;
        bool IsVariadic() const override;
        bool IsEmpty() const override { return isEmpty; }
        Expression* Call(FunctionOverload* function, ArrayView<Expression*> args, Expression* resultInst) override;
        void SetCallback(Callback callback) { fn = move(callback); }
        Callback GetCallback() const { return fn; }
    private:
        Function* owner;
        ModuleContext* mod;
        mutable FunctionPointerType* type;
        Callback fn;
        vector<ArgumentDeclaration> paramenterDeclarations;
        bool isEmpty;
    };

    struct DeletedFunctionOverload final : IObject::With<DeletedFunctionOverload, IFunctionOverload>
    {
        DeletedFunctionOverload(ModuleContext* mod, Function* owner, FunctionPointerType* type);
        Function* Owner() const override { return owner; }
        ModuleContext* Module() const override { return mod; }
        FunctionPointerType* FunctionType() const override { return type; }
        Llvm::Type* ReturnType() const override { return type->ReturnType(); }
        ArrayView<ArgumentDeclaration> ParameterDeclarations() const override { return paramenterDeclarations; }
        ArrayView<Llvm::Type*> ParameterTypes() const override { return type->ParameterTypes(); }
        bool IsVariadic() const override { return type->IsVariadic(); }
        Expression* Call(FunctionOverload* function, ArrayView<Expression*> args, Expression* resultInst) override;
    private:
        ModuleContext* mod;
        Function* owner;
        FunctionPointerType* type;
        vector<ArgumentDeclaration> paramenterDeclarations;
    };

    struct Function final : IObject::With<Function>
    {
        Function(ModuleContext* mod, string name);
        ArrayView<IFunctionOverload*> Overloads() const { return overloads; }
        string_view Name() const { return name; }
        void AddOverload(IFunctionOverload* overload);
        FunctionOverload* AddOverload(Llvm::Type* returnType, vector<ArgumentDeclaration> params, bool isVariadic = false, bool noMangling = false);
        IntrinsicFunction* AddIntrinsicOverload(Llvm::Type* returnType, vector<Llvm::Type*> paramTypes, bool isVariadic = false, const IntrinsicFunction::Callback& fn = IntrinsicFunction::Empty);
        IntrinsicFunction* AddIntrinsicOverload(FunctionPointerType* type, const IntrinsicFunction::Callback& fn = IntrinsicFunction::Empty);

        DeletedFunctionOverload* AddDeletedOverload(Llvm::Type* returnType, vector<Llvm::Type*> paramTypes, bool isVariadic = false);
        DeletedFunctionOverload* AddDeletedOverload(FunctionPointerType* type);

        Expression* Call(FunctionOverload* caller, ArrayView<Expression*> args, Expression* resultInst);

        vector<IFunctionOverload*> FindOverloads(ArrayView<Llvm::Type*> types) const;

        IFunctionOverload* FindOverload(ArrayView<Llvm::Type*> types) const;

        vector<IFunctionOverload*> FindCallable(ArrayView<Llvm::Type*> argTypes) const;

        IFunctionOverload* FindCallableSingle(ArrayView<Llvm::Type*> types) const;

        vector<pair<IFunctionOverload*, vector<IFunctionOverload*>>> FindConversionOverloads(ArrayView<Llvm::Type*> types) const;

        ModuleContext* Module() const override { return mod; }
    private:
        ModuleContext* mod;
        string name;
        vector<IFunctionOverload*> overloads;
    };

}
