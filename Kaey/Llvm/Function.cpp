#include "Function.hpp"
#include "Function.hpp"
#include "Function.hpp"
#include "ModuleContext.hpp"

#include "Types.hpp"

namespace Kaey::Llvm
{
    static string MangleName(std::string_view name, FunctionPointerType* type)
    {
        return "{}({}{})"_f(name, join(type->ParameterTypes() | name_of_range, ", "), type->IsVariadic() ? ", ..." : "");
    }

    VariableReference::VariableReference(ModuleContext* mod, FunctionOverload* function, string name, ReferenceType* type, llvm::Value* value) :
        ParentClass(mod, type, value),
        function(function)
    {
        value->setName(name);
        this->name = move(name);
    }

    FunctionOverload::FunctionOverload(ModuleContext* mod, Function* owner, vector<ArgumentDeclaration> paramDecl, Llvm::Type* returnType, bool isVariadic, bool noMangling) :
        owner(owner), mod(mod),
        functionType(mod->GetFunctionPointerType(returnType, paramDecl | vs::transform([](auto& arg) { return arg.Type; }) | to_vector, isVariadic)),
        type(functionType->Instance()),
        function(llvm::Function::Create(type, llvm::GlobalValue::ExternalLinkage, noMangling ? (string)owner->Name() : MangleName(owner->Name(), functionType), mod->Module())),
        parameterDeclarations(move(paramDecl)),
        entryBlock(nullptr), initBlock(nullptr),
        entryBuilder(nullptr), initBuilder(nullptr),
        structReturn(nullptr)
    {
        assert(mod == owner->Module());
        if (!functionType->ReturnType()->IsStructReturn())
            return;
        auto sret = function->getArg(0);
        sret->setName({});
        sret->addAttr(llvm::Attribute::NoAlias);
        sret->addAttr(llvm::Attribute::NoCapture);
        sret->addAttr(llvm::Attribute::getWithStructRetType(mod->Context(), sret->getType()));
    }

    Llvm::Type* FunctionOverload::ReturnType() const
    {
        return functionType->ReturnType();
    }

    ArrayView<Llvm::Type*> FunctionOverload::ParameterTypes() const
    {
        return functionType->ParameterTypes();
    }

    bool FunctionOverload::IsVariadic() const
    {
        return functionType->IsVariadic();
    }

    Expression* FunctionOverload::Call(FunctionOverload* caller, ArrayView<Expression*> args, Expression* resultInst)
    {
        auto voidType = Module()->GetVoidType();
        auto returnType = FunctionType()->ReturnType();
        auto parameterTypes = FunctionType()->ParameterTypes();
        vector<llvm::Value*> arr;
        auto min = std::min(parameterTypes.size(), args.size());
        for (size_t i = 0; i < min; i++)
            arr.emplace_back(parameterTypes[i]->PassArgument(caller, args[i])->Value());
        if (Type()->isVarArg())
        {
            auto n = args.size();
            arr.reserve(n);
            for (auto i = parameterTypes.size(); i < n; ++i)
            {
                auto arg = args[i];
                auto t = arg->Type()->RemoveReference();
                arr.emplace_back(t->PassArgument(caller, arg)->Value());
            }
        }
        else
        {
            auto max = std::max(parameterTypes.size(), args.size());
            for (size_t i = min; i < max; i++)
            {
                auto t = parameterTypes[i]->RemoveReference();
                arr.emplace_back(t->PassArgument(caller, parameterDeclarations[i].Initializer)->Value());
            }
        }

        if (returnType->IsStructReturn())
            arr.emplace(arr.begin(), resultInst->Value());
        auto v = caller->Builder()->CreateCall(function, arr);
        if (!returnType->IsStructReturn() && returnType != voidType && returnType->IsNot<ReferenceType>())
            caller->Builder()->CreateStore(v, resultInst->Value());
        return resultInst;
    }

    void FunctionOverload::Begin()
    {
        assert(!ended && "Function already defined!");
        if (scopeStack.empty())
        {
            entryBlock = llvm::BasicBlock::Create(mod->Context(), "Entry", Value());
            initBlock = llvm::BasicBlock::Create(mod->Context(), "Init", Value());
            auto block = llvm::BasicBlock::Create(mod->Context(), "Block", Value());
            scopeStack.emplace_back();

            entryBuilder = std::make_unique<IRBuilder>(entryBlock);
            initBuilder  = std::make_unique<IRBuilder>(initBlock);
            builder      = std::make_unique<IRBuilder>(block);

            entryBuilder->SetInsertPoint(entryBuilder->CreateBr(initBlock));
            initBuilder->SetInsertPoint(initBuilder->CreateBr(block));

            auto params = ParameterDeclarations();
            auto types = ParameterTypes();

            auto it = Value()->arg_begin();
            if (ReturnType()->IsStructReturn())
                structReturn = mod->CreateObject<Expression>(ReturnType()->ReferenceTo(), &*it++);
            for (size_t i = 0; i < params.size(); ++i)
                parameters.emplace_back(types[i]->CaptureArgument(this, params[i], &it[i]));
        }
        else
        {
            scopeStack.emplace_back();
        }
    }

    void FunctionOverload::End()
    {
        DestructScopeVariables();
        auto returned = scopeStack.back().Returned;
        scopeStack.pop_back();
        assert(!scopeStack.empty() || ReturnType()->Is<VoidType>() || returned && "Return statement is required for non void return types!");
        if (scopeStack.empty() && !returned && ReturnType()->Is<VoidType>())
            Builder()->CreateRetVoid();
        if (scopeStack.empty())
        {
            assert(whileStack.empty() && "Forgot to call 'EndWhile'!");
            assert(ifStack.empty() && "Forgot to call 'EndIf'!");
            assert(switchStack.empty() && "Forgot to call 'EndSwitch'!");
#ifdef _DEBUG
            ended = true;
#endif
        }
    }

    VariableReference* FunctionOverload::AllocateVariable(string name, Llvm::Type* type, int offset)
    {
        auto result = Module()->CreateObject<VariableReference>(this, move(name), type->ReferenceTo(), type->AllocateValue(this));
#ifdef _DEBUG
        if (result->Name() == "_")
            result->Value()->setName("v_{:04}"_f(++varCount));
#endif
        AddVariable(result, offset);
        return result;
    }

    void FunctionOverload::CreateReturn(Expression* expression)
    {
        auto& [vars, instances, returned, returning] = CurrentContext();
        assert(!returned && "Return already present on current scope!");
        returned = true;
        auto it = rn::find_if(vars, [=](auto& p) { return p.second == expression; });
        if (it != vars.end())
            vars.erase(it);
        DestructScopeVariables();
        auto builder = Builder();
        auto rt = ReturnType();
        if (rt->IsStructReturn())
        {
            rt->Construct(this, structReturn, { expression });
            builder->CreateRetVoid();
        }
        else if (rt->Is<ReferenceType>())
        {
            assert(expression->Type() == rt && "Different return types!");
            builder->CreateRet(expression->Value());
        }
        else
        {
            if (!expression)
            {
                assert(rt->Is<VoidType>());
                builder->CreateRet(nullptr);
                return;
            }
            auto v = builder->CreateLoad(expression->Type()->RemoveReference()->Instance(), expression->Value());
            builder->CreateRet(v);
        }
    }

    void FunctionOverload::BeginReturn()
    {
        auto& [vars, instances, returned, returning] = CurrentContext();
        assert(!returned || !returning && "Return already present on current scope!");
        returning = true;
    }

    void FunctionOverload::EndReturn()
    {
        auto& [vars, instances, returned, returning] = CurrentContext();
        assert(!returned && "Return already present on current scope!");
        assert(!returning && "'BeginReturn' must be called first!");
        returned = true;
        auto expr = instances.back();
        auto it = rn::find_if(vars, [=](auto& p) { return p.second == expr; });
        if (it != vars.end())
            vars.erase(it);
        DestructScopeVariables();
        auto builder = Builder();
        auto rt = ReturnType();
        if (rt->IsStructReturn())
        {
            rt->Construct(this, structReturn, { expr });
            builder->CreateRetVoid();
        }
        else if (rt->Is<ReferenceType>())
        {
            assert(expr->Type() == rt && "Different return types!");
            builder->CreateRet(expr->Value());
        }
        else
        {
            if (!expr)
            {
                assert(rt->Is<VoidType>());
                builder->CreateRet(nullptr);
                return;
            }
            auto v = builder->CreateLoad(expr->Type()->RemoveReference()->Instance(), expr->Value());
            builder->CreateRet(v);
        }
    }

    void FunctionOverload::AddOption(CallOptions option)
    {
        nextOption = move(option);
    }

    void FunctionOverload::BeginCall(Callee callee)
    {
        Begin();
        callStack.push({ callee, {  }, move(nextOption) });
        nextOption = nullptr;
    }

    void FunctionOverload::BeginCall(string_view callee)
    {
        return BeginCall(Module()->FindFunction(callee));
    }

    void FunctionOverload::AddCallParameter(Expression* expr)
    {
        assert(!callStack.empty() && "Use BeginCall first!");
        callStack.top().Arguments.emplace_back(expr);
    }

    Expression* FunctionOverload::EndCall()
    {
        assert(!callStack.empty() && "Use BeginCall first!");
        auto& [vari, args, options] = callStack.top();
        auto inst = visit(
            CreateOverload(
                [](Expression* inst) { return inst; },
                [](const string&) -> Expression* { return nullptr; },
                [](const ReturnCall&) -> Expression* { return nullptr; }
            ),
            options
        );
        auto alloc = [&](Llvm::Type* type)
        {
            return visit(
                CreateOverload(
                    [&](Expression* expr) { return expr ? expr : AllocateVariable("_", type, 1); },
                    [&](string& name) -> Expression* { return AllocateVariable(move(name), type, 1); },
                    [&](const ReturnCall&) -> Expression* { return structReturn ? structReturn : AllocateVariable("_", type, 1); }
                ),
                options
            );
        };
        auto f1 = [&](IFunctionOverload* callee)
        {
            auto retType = callee->ReturnType();
            assert(retType->IsNot<VoidType>() || !inst && "You may not elide a function that returns void!");
            assert(!inst || inst->Type() == retType->ReferenceTo() && "Return types differ!");
            if (retType->IsNot<VoidType>() && retType->IsNot<ReferenceType>() && !inst)
                inst = alloc(retType);
            return callee->Call(this, args, inst);
        };
        auto result = visit(
            CreateOverload(
                f1,
                [&](Function* callee)
                {
                    auto overloads = callee->FindCallable(args | type_of_range | to_vector);
                    if (overloads.empty() && callee->Name() == AMPERSAND_OPERATOR && args.size() == 1) //Default ampersand operator.
                        return CreateAddressOf(args[0]);
                    assert(!overloads.empty() && "No valid overload found!");
                    assert(overloads.size() == 1 && "Multiples overloads found!");
                    return f1(overloads[0]);
                },
                [&](Llvm::Type* callee)
                {
                    if (!inst) inst = alloc(callee);
                    callee->Construct(this, inst, args);
                    return inst;
                }
            ),
            vari
        );

        callStack.pop();
        End();

        if (holds_alternative<ReturnCall>(options))
            CreateReturn(inst ? inst : result);

        return result;
    }

    Expression* FunctionOverload::AddExpression(Expression* expr)
    {
        auto nOpt = move(nextOption);
        nextOption = nullptr;
        visit(CreateOverload(
                [=, this](Expression* inst)
                {
                    if (!inst) return;
                    auto type = inst->Type()->As<ReferenceType>()->UnderlyingType();
                    AddOption(inst);
                    BeginCall(type);
                        AddCallParameter(expr);
                    EndCall();
                },
                [=, this](string& name)
                {
                    AddOption(move(name));
                    BeginCall(expr->Type()->As<ReferenceType>()->UnderlyingType());
                        AddCallParameter(expr);
                    EndCall();

                },
                [=, this](const ReturnCall&)
                {
                    CreateReturn(expr);
                }
            ),
            nOpt
        );
        return expr;
    }

    Expression* FunctionOverload::CreateAddressOf(Expression* inst)
    {
        assert(inst->Type()->Is<ReferenceType>() && "Instance must be a reference!");
        auto ptrType = inst->Type()->As<ReferenceType>()->UnderlyingType()->PointerTo();
        auto result = AllocateVariable("_", ptrType);
        Builder()->CreateStore(inst->Value(), result->Value());
        return result;
    }

    Expression* FunctionOverload::CallUnscoped(Callee callee)
    {
        BeginCall(callee);
        return EndCall();
    }

    void FunctionOverload::BeginIf(Llvm::Type* retType)
    {
        Begin();
        ifStack.push({
            retType,
            nullptr,
            llvm::BasicBlock::Create(mod->Context(), {}, function),
            llvm::BasicBlock::Create(mod->Context(), {}, function),
            nullptr
        });
    }

    void FunctionOverload::AddIfCondition(Expression* cond)
    {
        assert(!ifStack.empty() && "Use 'BeginIf' first!");
        auto& [rt, ic, tb, fb, eb] = ifStack.top();
        assert(!ic && "Condition is already set!");
        ic = cond->Value();
        //TODO Convert condition to boolean.
        ic = Builder()->CreateLoad(Module()->GetBooleanType()->Instance(), cond->Value());
        Builder()->CreateCondBr(ic, tb, fb);
        Builder()->SetInsertPoint(tb);
    }

    void FunctionOverload::BeginElse()
    {
        auto& [rt, ic, tb, fb, eb] = ifStack.top();
        assert(!eb && "'BeginElse' has already been called!");
        eb = llvm::BasicBlock::Create(mod->Context(), {}, function);
        std::swap(fb, eb);
        auto returned = CurrentContext().Returned;
        End();
        if (!returned)
            Builder()->CreateBr(fb);
        Builder()->SetInsertPoint(eb);
        Begin();
    }

    void FunctionOverload::EndIf()
    {
        auto returned = CurrentContext().Returned;
        End();
        auto& [rt, ic, tb, fb, eb] = ifStack.top();
        if (!returned)
            Builder()->CreateBr(fb);
        Builder()->SetInsertPoint(fb);
        ifStack.pop();
    }

    void FunctionOverload::BeginWhile()
    {
        Begin();
        whileStack.push({
            nullptr,
            llvm::BasicBlock::Create(mod->Context(), {}, function),
            llvm::BasicBlock::Create(mod->Context(), {}, function),
            llvm::BasicBlock::Create(mod->Context(), {}, function)
        });
        auto& [ic, cb, tb, fb] = whileStack.top();
        Builder()->CreateBr(cb);
        Builder()->SetInsertPoint(cb);
    }

    void FunctionOverload::AddWhileCondition(Expression* cond)
    {
        assert(!whileStack.empty() && "Use 'BeginWhile' first!");
        auto& [ic, cb, tb, fb] = whileStack.top();
        assert(!ic && "Condition is already set!");
        ic = cond->Value();
        //TODO Convert condition to boolean.
        ic = Builder()->CreateLoad(Module()->GetBooleanType()->Instance(), cond->Value());
        Builder()->CreateCondBr(ic, tb, fb);
        Builder()->SetInsertPoint(tb);
    }

    void FunctionOverload::EndWhile()
    {
        auto returned = CurrentContext().Returned;
        End();
        auto& [wc, cb, tb, fb] = whileStack.top();
        if (!returned)
            Builder()->CreateBr(cb);
        Builder()->SetInsertPoint(fb);
        whileStack.pop();
    }

    void FunctionOverload::BeginSwitch()
    {
        Begin();
        switchStack.push({
            nullptr,
            nullptr,
            {},
            llvm::BasicBlock::Create(mod->Context(), {}, function),
            llvm::BasicBlock::Create(mod->Context(), {}, function)
        });
    }

    void FunctionOverload::AddSwitchCondition(Expression* cond)
    {
        assert(!switchStack.empty() && "Use 'BeginSwitch' first!");
        auto& [sc, si, cases, db, eb] = switchStack.top();
        assert(!sc && "Condition is already set!");
        auto u64 = Module()->GetIntegerType(64, false);
        BeginCall(u64);
            AddCallParameter(cond);
        sc = EndCall()->Value();
        sc = Builder()->CreateLoad(u64->Instance(), sc);
        si = Builder()->CreateSwitch(sc, db, 0);
    }

    void FunctionOverload::BeginCase()
    {
        Begin();
    }

    void FunctionOverload::AddCaseValue(Constant* v)
    {
        assert(!switchStack.empty() && "Use 'BeginSwitch' first!");
        auto& [sc, si, cases, db, eb] = switchStack.top();
        if (v)
        {
            auto u64 = mod->GetIntegerType(64, false);
            auto ty = v->Type()->RemoveReference()->As<IntegerType>();
            assert(ty);
            cases.emplace_back(u64->CreateConstantValueUnsigned(ty->GetConstantValue(v)), llvm::BasicBlock::Create(mod->Context(), {}, function, db));
            auto& [e, s] = cases.back();
            si->addCase(e, s);
            Builder()->SetInsertPoint(s);
        }
        else
        {
            cases.emplace_back(nullptr, db);
            Builder()->SetInsertPoint(db);
        }
    }

    void FunctionOverload::EndCase()
    {
        auto& [sc, si, cases, db, eb] = switchStack.top();
        if (!CurrentContext().Returned)
            Builder()->CreateBr(eb);
        End();
    }

    void FunctionOverload::EndSwitch()
    {
        assert(!switchStack.empty() && "Use 'BeginSwitch' first!");
        auto& [sc, si, cases, db, eb] = switchStack.top();
        if (rn::none_of(cases, [](auto& p) { return !p.first; }))
        {
            BeginCase();
            AddCaseValue(nullptr);
            EndCase();
        }
        Builder()->SetInsertPoint(eb);
        End();
        switchStack.pop();
    }

    Expression* FunctionOverload::Malloc(Llvm::Type* type)
    {
        auto ity = mod->GetIntegerType(sizeof size_t * 8, true);
        auto malloc = mod->FindFunction("malloc");
        if (!malloc)
            malloc = mod->DeclareFunction("malloc", { { .Type = ity } }, mod->GetVoidType()->PointerTo(), false, true)->Owner();
        auto size = (int64_t)mod->DataLayout()->getTypeAllocSize(type->Instance());
        BeginCall(malloc);
            AddCallParameter(ity->CreateConstant(size));
        auto ptr = EndCall();
        auto result = AllocateVariable({}, type->PointerTo());
        Builder()->CreateStore(Builder()->CreatePointerCast(Builder()->CreateLoad(mod->GetVoidType()->PointerTo()->Instance(), ptr->Value()), type->PointerTo()->Instance()), result->Value());
        return result;
    }

    Expression* FunctionOverload::Free(Expression* ptr)
    {
        auto vTy = mod->GetVoidType();
        auto free = mod->FindFunction("free");
        if (!free)
            free = mod->DeclareFunction("free", { { .Type = vTy->PointerTo() } }, vTy->PointerTo(), false, true)->Owner();
        BeginCall(free);
            AddCallParameter(ptr);
        return EndCall();
    }

    Expression* FunctionOverload::GetField(Expression* owner, int index)
    {
        return owner->GetField(this, index);
    }

    Expression* FunctionOverload::PointerCast(Expression* ptr, Llvm::Type* type)
    {
        auto ty = ptr->Type();
        assert(ty->Is<PointerType>() || ty->Is<ReferenceType>());
        ty = ty->Is<PointerType>() ? (Llvm::Type*)type->PointerTo() : type->ReferenceTo();
        auto value = Builder()->CreateBitOrPointerCast(ptr->Value(), ty->Instance());
        return Module()->CreateObject<Expression>(ty, value);
    }

    void FunctionOverload::Construct(Expression* inst, vector<Expression*> args)
    {
        auto type = inst->Type();
        assert(type->Is<ReferenceType>() || type->Is<PointerType>());
        type = type->RemoveReference()->RemovePointer();
        type->Construct(this, inst, move(args));
    }

    void FunctionOverload::Destruct(Expression* inst)
    {
        auto type = inst->Type();
        assert(type->Is<ReferenceType>() || type->Is<PointerType>());
        type = type->RemoveReference()->RemovePointer();
        type->Destruct(this, inst);
    }

    FunctionOverload::BlockContext& FunctionOverload::CurrentContext()
    {
        return scopeStack.back();
    }

    void FunctionOverload::AddVariable(VariableReference* v, ptrdiff_t offset)
    {
        auto& vars = scopeStack[scopeStack.size() - (offset + 1)].Variables;
        assert(v->Name() == "_" || !vars.contains(v->Name()) && "Identifier already present on scope!");
        vars.emplace(v->Name(), v);
    }

    void FunctionOverload::DestructScopeVariables()
    {
        for (auto& v : scopeStack.back().Variables | vs::values)
            Destruct(v);
        scopeStack.back().Variables.clear();
    }

    const IntrinsicFunction::Callback IntrinsicFunction::Empty = [](FunctionOverload*, ArrayView<Expression*>, Expression*) { return nullptr; };

    IntrinsicFunction::IntrinsicFunction(ModuleContext* mod, Function* owner, FunctionPointerType* type, const Callback& fn) :
        owner(owner),
        mod(mod),
        type(type),
        fn(fn),
        paramenterDeclarations(type->ParameterTypes() | vs::transform([](Llvm::Type* ty) { return ArgumentDeclaration({}, ty, nullptr); }) | to_vector),
        isEmpty(&fn == &Empty)
    {
        assert(mod == owner->Module());
    }

    Llvm::Type* IntrinsicFunction::ReturnType() const
    {
        return type->ReturnType();
    }

    ArrayView<Llvm::Type*> IntrinsicFunction::ParameterTypes() const
    {
        return type->ParameterTypes();
    }

    bool IntrinsicFunction::IsVariadic() const
    {
        return type->IsVariadic();
    }

    Expression* IntrinsicFunction::Call(FunctionOverload* function, ArrayView<Expression*> args, Expression* resultInst)
    {
        return fn(function, args, resultInst);
    }

    DeletedFunctionOverload::DeletedFunctionOverload(ModuleContext* mod, Function* owner, FunctionPointerType* type) :
        mod(mod),
        owner(owner),
        type(type),
        paramenterDeclarations(type->ParameterTypes() | vs::transform([](Llvm::Type* ty) { return ArgumentDeclaration({}, ty, nullptr); }) | to_vector)
    {
        assert(mod == owner->Module() && "Modules from argument and owner differ!");
    }

    Expression* DeletedFunctionOverload::Call(FunctionOverload* function, ArrayView<Expression*> args, Expression* resultInst)
    {
        assert(false && "Deleted Function may not be called!");
        return nullptr;
    }

    Function::Function(ModuleContext* mod, string name) : mod(mod), name(move(name))
    {
        
    }

    void Function::AddOverload(IFunctionOverload* overload)
    {
#ifdef _DEBUG
        auto f = FindOverload(overload->ParameterTypes());
        assert(!f || f->ParameterTypes() != overload->ParameterTypes());
#endif
        overloads.emplace_back(overload);
    }

    FunctionOverload* Function::AddOverload(Llvm::Type* returnType, vector<ArgumentDeclaration> params, bool isVariadic, bool noMangling)
    {
        auto result = mod->CreateObject<FunctionOverload>(this, move(params), returnType, isVariadic, noMangling);
        AddOverload(result);
        return result;
    }

    IntrinsicFunction* Function::AddIntrinsicOverload(Llvm::Type* returnType, vector<Llvm::Type*> paramTypes, bool isVariadic, const IntrinsicFunction::Callback& fn)
    {
        auto type = mod->GetFunctionPointerType(returnType, move(paramTypes), isVariadic);
        return AddIntrinsicOverload(type, fn);
    }

    IntrinsicFunction* Function::AddIntrinsicOverload(FunctionPointerType* type, const IntrinsicFunction::Callback& fn)
    {
        auto result = mod->CreateObject<IntrinsicFunction>(this, type, fn);
        AddOverload(result);
        return result;
    }

    DeletedFunctionOverload* Function::AddDeletedOverload(Llvm::Type* returnType, vector<Llvm::Type*> paramTypes, bool isVariadic)
    {
        return AddDeletedOverload(Module()->GetFunctionPointerType(returnType, move(paramTypes), isVariadic));
    }

    DeletedFunctionOverload* Function::AddDeletedOverload(FunctionPointerType* type)
    {
        return Module()->CreateObject<DeletedFunctionOverload>(this, type);
    }

    Expression* Function::Call(FunctionOverload* caller, ArrayView<Expression*> args, Expression* resultInst)
    {
        auto types = args | type_of_range | to_vector;
        auto overs = FindCallable(types);
        assert(!overs.empty() && "No valid overload found!");
        return overs[0]->Call(caller, args, resultInst);
    }

    vector<IFunctionOverload*> Function::FindOverloads(ArrayView<Llvm::Type*> types) const
    {
        vector<IFunctionOverload*> result;
        for (auto overload : overloads)
        {
            auto params = overload->ParameterTypes();
            if (params.size() > types.size() ||
                params.size() < types.size() && !overload->IsVariadic() ||
                !std::equal(params.begin(), params.end(), types.begin(), [](Llvm::Type* lhs, Llvm::Type* rhs) { return lhs == rhs || lhs->RemoveReference() == rhs->RemoveReference(); }))
                continue;
            result.emplace_back(overload);
        }
        return result;
    }

    IFunctionOverload* Function::FindOverload(ArrayView<Llvm::Type*> types) const
    {
        auto it = rn::find_if(overloads, [&](auto o) { return o->ParameterTypes() == types; });
        return it != overloads.end() ? *it : nullptr;
    }

    vector<IFunctionOverload*> Function::FindCallable(ArrayView<Llvm::Type*> argTypes) const
    {
        for (auto overload : overloads)
        {
            auto paramTypes = overload->ParameterTypes();
            if (rn::equal(paramTypes, argTypes, [](Llvm::Type* a, Llvm::Type* b) { return a == b || a->RemoveReference() == b->RemoveReference(); }))
                return { overload };
        }
        vector<IFunctionOverload*> res;
        for (auto overload : overloads)
        {
            auto ty = overload->FunctionType();
            auto paramTypes = overload->ParameterTypes();

            if (argTypes.size() > paramTypes.size() && !ty->IsVariadic())
                continue;
            auto params = overload->ParameterDeclarations();
            if (argTypes.size() < paramTypes.size() && rn::any_of(params | vs::drop(argTypes.size()), [](const ArgumentDeclaration& pd)
                {
                    return pd.Initializer == nullptr;
                }))
                continue;
            auto n = std::min(paramTypes.size(), argTypes.size());
            if (!rn::equal(paramTypes | vs::take(n), argTypes | vs::take(n), [](Llvm::Type* a, Llvm::Type* b)
                {
                    if (a == b || a->RemoveReference() == b->RemoveReference())
                        return true;
                    auto cc = a->FindFunction(CONSTRUCTOR);
                    return cc && (cc->FindOverload({ a->ReferenceTo(), b }) || cc->FindOverload({ a->ReferenceTo(), b->RemoveReference() }));
                }))
                continue;
            res.emplace_back(overload);

            //if (pTypes.size() > argTypes.size() ||
            //    pTypes.size() < argTypes.size() && !overload->IsVariadic())
            //    continue;
            //if (std::equal(pTypes.begin(), pTypes.end(), argTypes.begin(), [](auto a, auto b) { return a == b || a->RemoveReference() == b->RemoveReference(); }))
            //    return { overload };
            //if (std::equal(pTypes.begin(), pTypes.end(), argTypes.begin(), [](auto a, auto b)
            //    {
            //        auto cc = a->FindFunction(CONSTRUCTOR);
            //        return cc && (cc->FindOverload({ a->ReferenceTo(), b }) || cc->FindOverload({ a->ReferenceTo(), b->RemoveReference() }));
            //    }))
            //    res.emplace_back(overload);
        }
        return res;
    }

    IFunctionOverload* Function::FindCallableSingle(ArrayView<Llvm::Type*> types) const
    {
        auto v = FindCallable(types);
        return !v.empty() ? v[0] : nullptr;
    }

    vector<pair<IFunctionOverload*, vector<IFunctionOverload*>>> Function::FindConversionOverloads(ArrayView<Llvm::Type*> types) const
    {
        vector<pair<IFunctionOverload*, vector<IFunctionOverload*>>> results;
        for (auto overload : overloads)
        {
            auto pTypes = overload->ParameterTypes();
            if (pTypes.size() > types.size() ||
                pTypes.size() < types.size() && !overload->IsVariadic())
                continue;
            vector<IFunctionOverload*> converts;
            for (size_t i = 0; i < types.size(); i++)
            {
                if (types[i] == pTypes[i])
                {
                    converts.emplace_back(nullptr);
                    continue;
                }
                assert(pTypes[i]->IsNot<ReferenceType>());
                if (auto cons = pTypes[i]->FindFunction(CONSTRUCTOR))
                if (auto cFn = cons->FindOverload({ pTypes[i]->ReferenceTo(), types[i] }))
                if (cFn->IsImplicit())
                {
                    converts.emplace_back(cFn);
                    continue;
                }
                goto end;
            }
            results.emplace_back(overload, move(converts));
        end:
            (void)0;
        }
        return results;
    }

}
