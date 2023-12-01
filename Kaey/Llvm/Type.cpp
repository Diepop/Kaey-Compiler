#include "Type.hpp"

#include "Function.hpp"
#include "ModuleContext.hpp"

#include "Types.hpp"

namespace Kaey::Llvm
{
    Type::Type(ModuleContext* mod, string name, llvm::Type* type) : mod(mod), name(move(name)), type(type)
    {

    }

    llvm::Value* Type::AllocateValue(FunctionOverload* fn)
    {
        return fn->EntryBuilder()->CreateAlloca(Instance(), nullptr, {});
    }

    Expression* Type::Allocate(FunctionOverload* fn)
    {
        return mod->CreateObject<Expression>(ReferenceTo(), AllocateValue(fn));
    }

    void Type::Construct(FunctionOverload* caller, Expression* inst, vector<Expression*> args)
    {
        assert(inst->Type()->Is<ReferenceType>() && "Instance must be a reference!");
        args.emplace(args.begin(), inst);
        auto types = args | type_of_range | to_vector;
        auto type = inst->Type()->RemoveReference();
        auto fn = type->FindFunction(CONSTRUCTOR);
        assert(fn && "No contructor found");
        fn->Call(caller, args, nullptr);
    }

    void Type::Destruct(FunctionOverload* caller, Expression* inst)
    {
        assert(inst->Type()->Is<ReferenceType>() && "Instance must be a reference!");
        auto type = inst->Type()->RemoveReference();
        auto fn = type->FindFunction(DESTRUCTOR);
        assert(fn && "No destructor found");
        fn->Call(caller, { inst }, nullptr);
    }

    void Type::DeclareMethods()
    {
        auto emptyFields = [](ArrayView<FieldDeclaration*> fds, string_view name, int v)
        {
            return fds.empty() || rn::all_of(fds, [&](FieldDeclaration* fd)
            {
                auto cc = fd->Type->FindFunction(name);
                if (!cc)
                    return false;
                auto types = v == 0 ? vector<Type*>{ fd->Type->ReferenceTo() } : vector<Type*>{ fd->Type->ReferenceTo(), fd->Type->ReferenceTo() };
                auto fn = cc->FindOverload(types);
                return fn && fn->IsEmpty();
            });
        };
        auto voidTy = mod->GetVoidType();
        auto refTy = ReferenceTo();
        auto constructor = FindFunction(CONSTRUCTOR);
        if (!constructor)
        {
            constructor = AddFunction(CONSTRUCTOR);
            if (emptyFields(Fields(), CONSTRUCTOR, 0))
                constructor->AddInstrinsicOverload(voidTy, { refTy, refTy }, false, InstrinsicFunction::Empty);
            else AddFunctionDeclaration(constructor, { { { .Type = refTy } } }, [&](auto fn)
            {
                auto this_ = fn->Parameter(0);
                for (auto fd : Fields())
                    fn->Construct(fn->GetField(this_, fd->Index), {});
            });
            if (!Fields().empty())
            {
                auto tys = vector<ArgumentDeclaration>{ { .Type = refTy } };
                tys.reserve(Fields().size() + 1);
                for (auto f : Fields())
                    tys.push_back({ .Type = f->Type });
                AddFunctionDeclaration(constructor, { move(tys) }, [&](FunctionOverload* fn)
                {
                    auto this_ = fn->Parameter(0);
                    for (auto f : Fields())
                        fn->Construct(fn->GetField(this_, f->Index), { fn->Parameter(f->Index + 1) });
                });
            }
        }
        auto copyConstructor = constructor->FindOverload({ refTy, refTy });
        if (!copyConstructor)
        {
            if (emptyFields(Fields(), CONSTRUCTOR, 1))
                constructor->AddInstrinsicOverload(voidTy, { refTy, refTy }, false, InstrinsicFunction::Empty);
            else AddFunctionDeclaration(constructor, { { { .Type = refTy }, { .Type = refTy } } }, [&](auto fn)
            {
                auto [lhs, rhs] = fn->UnpackParameters<2>();
                for (auto fd : Fields())
                    fn->Construct(fn->GetField(lhs, fd->Index), { fn->GetField(rhs, fd->Index) });
            });
        }
        auto assign = FindFunction(ASSIGN_OPERATOR);
        auto copyAssign = assign->FindOverload({ refTy, refTy });
        if (!copyAssign)
        {
            if (emptyFields(Fields(), ASSIGN_OPERATOR, 1))
                assign->AddInstrinsicOverload(voidTy, { refTy, refTy }, false, InstrinsicFunction::Empty);
            else AddFunctionDeclaration(assign, { { { .Type = refTy }, { .Type = refTy } }, refTy }, [&](auto fn)
            {
                auto [lhs, rhs] = fn->UnpackParameters<2>();
                for (auto fd : Fields())
                {
                    fn->BeginCall(ASSIGN_OPERATOR);
                    fn->AddCallParameter(fn->GetField(lhs, fd->Index));
                    fn->AddCallParameter(fn->GetField(rhs, fd->Index));
                    fn->EndCall();
                }
                fn->CreateReturn(lhs);
            });
        }
        auto destructorFn = FindFunction(DESTRUCTOR);
        if (!destructorFn)
            destructorFn = AddFunction(DESTRUCTOR);
        auto destructor = destructorFn->FindOverload({ refTy });
        if (!destructor)
        {
            destructor = emptyFields(Fields(), DESTRUCTOR, 0) ?
                (IFunctionOverload*)destructorFn->AddInstrinsicOverload(voidTy, { refTy }, false, InstrinsicFunction::Empty) :
                AddFunctionDeclaration(destructorFn, { { { .Type = refTy } } }, [&](auto fn)
                {
                    for (auto fd : Fields() | vs::reverse)
                        fn->Destruct(fn->GetField(fn->Parameter(0), fd->Index));
                });
        }
        auto boolTy = mod->GetBooleanType();
        auto equalOp = FindFunction(EQUAL_OPERATOR);
        auto notEqualOp = FindFunction(NOT_EQUAL_OPERATOR);
        if (!equalOp)
            equalOp = AddFunction(EQUAL_OPERATOR);
        if (!notEqualOp)
            equalOp = AddFunction(NOT_EQUAL_OPERATOR);
        
        auto equal = equalOp->FindCallableSingle({ refTy, refTy });
        auto notEqual = notEqualOp->FindCallableSingle({ refTy, refTy });

        if (!equal)
        {
            if (notEqual)
                equal = AddFunctionDeclaration(equalOp, { { { .Type = refTy }, {.Type = refTy } }, boolTy }, [&](FunctionOverload* fn)
                {
                    auto [lhs, rhs] = fn->UnpackParameters<2>();
                    fn->AddOption(ReturnCall());
                    fn->BeginCall(NOT_OPERATOR);
                        fn->BeginCall(NOT_EQUAL_OPERATOR);
                            fn->AddCallParameter(lhs);
                            fn->AddCallParameter(rhs);
                        fn->AddCallParameter(fn->EndCall());
                    fn->CreateReturn(fn->EndCall());
                });
            else if (!emptyFields(Fields(), EQUAL_OPERATOR, 1))
                equal = AddFunctionDeclaration(equalOp, { { { .Type = refTy }, { .Type = refTy } }, boolTy }, [=](FunctionOverload* fn)
                {
                    auto [lhs, rhs] = fn->UnpackParameters<2>();
                    fn->BeginIf();
                    for (auto fd : Fields())
                    {
                        fn->BeginCall(EQUAL_OPERATOR);
                            fn->AddCallParameter(fn->GetField(lhs, fd->Index));
                            fn->AddCallParameter(fn->GetField(rhs, fd->Index));
                        fn->AddIfCondition(fn->EndCall());
                        if (fd->Index + 1 < Fields().size())
                            fn->BeginIf();
                    }
                    fn->CreateReturn(boolTy->CreateConstant(true));
                    for (auto fd : Fields())
                        fn->EndIf();
                    fn->CreateReturn(boolTy->CreateConstant(false));
                });
        }

        if (!notEqual)
        {
            if (this->IsOr<PointerType>())
                notEqual = notEqualOp->AddInstrinsicOverload(boolTy, { this, this }, false,
                    [=](FunctionOverload* caller, ArrayView<Expression*> args, Expression* result) -> Expression*
                    {
                        auto builder = caller->Builder();
                        auto v = builder->CreateICmpNE(args[0]->Value(), args[1]->Value());
                        builder->CreateStore(v, result->Value());
                        return result;
                    }
                );
            else if (equal)
                notEqual = AddFunctionDeclaration(notEqualOp, { { { .Type = refTy }, { .Type = refTy } }, boolTy }, [&](FunctionOverload* fn)
                {
                    auto [lhs, rhs] = fn->UnpackParameters<2>();
                    fn->BeginCall(NOT_OPERATOR);
                        fn->BeginCall(EQUAL_OPERATOR);
                            fn->AddCallParameter(lhs);
                            fn->AddCallParameter(rhs);
                        fn->AddCallParameter(fn->EndCall());
                    fn->CreateReturn(fn->EndCall());
                });
        }

        auto amper = FindFunction(AMPERSAND_OPERATOR);
        if (!amper)
        {
            amper = AddFunction(AMPERSAND_OPERATOR);
            assign->AddInstrinsicOverload(PointerTo(), { refTy }, false,
                [=, this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
                {
                    return caller->CreateAddressOf(args[0]);
                }
            );
        }
    }

    void Type::CreateMethods()
    {
        for (auto& f : createFns)
            f();
        createFns.clear();
    }

    ArgumentReference* Type::CaptureArgument(FunctionOverload* function, const ArgumentDeclaration& declaration, llvm::Argument* arg)
    {
        arg->setName(declaration.Name);
        return mod->CreateObject<ArgumentReference>(function, declaration.Name, declaration.Type->ReferenceTo(), arg);
    }

    Expression* Type::PassArgument(FunctionOverload* fn, Expression* expr)
    {
        auto inst = Allocate(fn);
        Construct(fn, inst, { expr });
        return inst;
    }

    llvm::Type* Type::TypeOfParameter() const
    {
        return Instance()->getPointerTo();
    }

    llvm::Type* Type::TypeOfReturn() const
    {
        return llvm::Type::getVoidTy(Module()->Context());
    }

    bool Type::IsStructReturn() const
    {
        return true;
    }

    ReferenceType* Type::ReferenceTo()
    {
        return Module()->GetReferenceType(this);
    }

    PointerType* Type::PointerTo()
    {
        return Module()->GetPointerType(this);
    }

    OptionalType* Type::OptionalTo()
    {
        return Module()->GetOptionalType(this);
    }

    Type* Type::RemoveReference()
    {
        return this;
    }

    Type* Type::RemovePointer()
    {
        return this;
    }

    bool Type::IsTriviallyCopyable() const
    {
        return true;
    }

    int64_t Type::Alignment() const
    {
        return (int64_t)Module()->DataLayout()->getABITypeAlignment(Instance());
    }

    int64_t Type::Size() const
    {
        return (int64_t)Module()->DataLayout()->getTypeAllocSize(Instance());
    }

    Function* Type::AddFunction(string name)
    {
        assert(name.rfind("operator") != 0 && "Operators are avaliable in the module context only!");
        //assert(name[0] == '_' || std::isalpha(name[0]) && "Function names must begin with either underscore or a letter!");
        //assert(std::all_of(name.begin() + 1, name.end(), [](char d) { return d == '_' || std::isalnum(d); }) && "Function names must not contain special characters!");
        return AddFunctionUnchecked(move(name));
    }

    Function* Type::FindFunction(string_view name)
    {
        auto it = functionMap.find((string)name);
        auto result = it != functionMap.end() ? it->second : nullptr;
        return result ? result : Module()->FindFunction(name);
    }

    const FieldDeclaration* Type::FindField(string_view name)
    {
        auto it = fieldMap.find(name);
        return it != fieldMap.end() ? it->second : nullptr;
    }

    Expression* Type::GetField(FunctionOverload* caller, Expression* inst, int index)
    {
        assert(ReferenceTo() == inst->Type() && "Instance must be a reference to this type!");
        assert(index >= 0 && "Index cannot be a negative number!");
        assert(index < (int)fields.size() && "Index out of bounds!");
        auto ty = inst->Type()->RemoveReference();
        auto gep = caller->Builder()->CreateStructGEP(ty->Instance(), inst->Value(), index);
        return Module()->CreateObject<Expression>(fields[index]->Type->ReferenceTo(), gep);
    }

    IFunctionOverload* Type::FindMethodOverload(string_view name, vector<Type*> paramTypes)
    {
        auto fn = FindFunction(name);
        assert(fn && "Function not found!");
        paramTypes.emplace(paramTypes.begin(), ReferenceTo());
        return fn->FindOverload(paramTypes);
    }

    void Type::AddField(string name, Type* type)
    {
        assert(type != this && "Recursive type declared!");
        assert(FindField(name) == nullptr && "A field with the same name is already defined!");
        auto ptr = fieldInsts.emplace_back(std::make_unique<FieldDeclaration>(move(name), type, (int)fields.size())).get();
        auto [it, _] = fieldMap.emplace(ptr->Name, ptr);
        fields.emplace_back(ptr);
    }

    FunctionOverload* Type::AddFunctionDeclaration(Function* fn, FunctionDeclarationArgs args, std::function<void(FunctionOverload*)> declFn)
    {
        auto f = fn->AddOverload(args.ReturnType ? args.ReturnType : Module()->GetVoidType(), move(args.Arguments), args.IsVariadic, args.NoMangling);
        createFns.emplace_back([=]
        {
            f->Begin();
            declFn(f);
            f->End();
        });
        return f;
    }

    Function* Type::AddFunctionUnchecked(string name)
    {
        assert(!FindFunction(name) && "Function already added!");
        auto result = mod->CreateObject<Function>("{}.{}"_f(Name(), name));
        functions.emplace_back(result);
        functionMap.emplace(move(name), result);
        return result;
    }

    ArgumentReference* ValueParameterType::CaptureArgument(FunctionOverload* fn, const ArgumentDeclaration& declaration, llvm::Argument* arg)
    {
        auto inst = AllocateValue(fn);
        fn->InitBuilder()->CreateStore(arg, inst);
        arg->setName("${}"_f(declaration.Name));
        inst->setName(declaration.Name);
        return Module()->CreateObject<ArgumentReference>(fn, declaration.Name, declaration.Type->ReferenceTo(), inst);
    }

    Expression* ValueParameterType::PassArgument(FunctionOverload* fn, Expression* expr)
    {
        if (this != expr->Type()->RemoveReference())
        {
            fn->BeginCall(this);
                fn->AddCallParameter(expr);
            return PassArgument(fn, fn->EndCall());
        }
        auto v = fn->Builder()->CreateLoad(expr->Type()->RemoveReference()->Instance(), expr->Value());
        return Module()->CreateObject<Expression>(this, v);
    }

    llvm::Type* ValueParameterType::TypeOfParameter() const
    {
        return Instance();
    }

    llvm::Type* ValueParameterType::TypeOfReturn() const
    {
        return Instance();
    }

    bool ValueParameterType::IsStructReturn() const
    {
        return false;
    }
}
