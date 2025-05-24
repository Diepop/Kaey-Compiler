#include "Types.hpp"

#include "Function.hpp"
#include "ModuleContext.hpp"

#define CREATE_BINARY_OPERATOR(f) \
[=](FunctionOverload* caller, ArrayView<Expression*> args, Expression* result) -> Expression* { \
    auto builder = caller->Builder(); \
    auto lhs = builder->CreateLoad(args[0]->Type()->RemoveReference()->Instance(), args[0]->Value()); \
    auto rhs = builder->CreateLoad(args[1]->Type()->RemoveReference()->Instance(), args[1]->Value()); \
    auto v = builder->f(lhs, rhs); \
    builder->CreateStore(v, result->Value()); \
    return result; \
}

#define CREATE_BINARY_OPERATOR_SU(fs, fu) (IsSigned() ? (IntrinsicFunction::Callback)CREATE_BINARY_OPERATOR(fs) : CREATE_BINARY_OPERATOR(fu))

#define CREATE_ASSIGNMENT_OPERATOR(f) \
[=](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression* { \
    auto builder = caller->Builder(); \
    auto lhs = builder->CreateLoad(args[0]->Type()->RemoveReference()->Instance(), args[0]->Value()); \
    auto rhs = builder->CreateLoad(args[1]->Type()->RemoveReference()->Instance(), args[1]->Value()); \
    auto v = builder->f(lhs, rhs); \
    builder->CreateStore(v, args[0]->Value()); \
    return args[0]; \
}

#define CREATE_ASSIGNMENT_OPERATOR_SU(fs, fu) (IsSigned() ? (IntrinsicFunction::Callback)CREATE_ASSIGNMENT_OPERATOR(fs) : CREATE_ASSIGNMENT_OPERATOR(fu))

namespace Kaey::Llvm
{

    ArrayType::ArrayType(ModuleContext* mod, Type* underlyingType, int64_t length) :
        ParentClass(mod, "{}[{}]"_f(underlyingType->Name(), length), llvm::ArrayType::get(underlyingType->Instance(), (uint64_t)length)),
        underlyingType(underlyingType), length(length)
    {

    }

    void ArrayType::DeclareMethods()
    {
        FindFunction(PLUS_OPERATOR)->AddIntrinsicOverload(UnderlyingType()->PointerTo(), { ReferenceTo() })->SetCallback(
            [this](FunctionOverload* caller, ArrayView<Expression*> args, Expression* inst) -> Expression*
            {
                auto ptrType = UnderlyingType()->PointerTo();
                auto builder = caller->Builder();
                auto v = builder->CreateBitCast(args[0]->Value(), ptrType->Instance());
                builder->CreateStore(v, inst->Value());
                return inst;
            }
        );
        //auto mod = Module();
        //auto index = FindFunction(INDEX_OPERATOR);
        //for (auto isSigned : { true, false })
        //for (auto bits : { 8, 16, 32, 64 })
        //    index->AddIntrinsicOverload(UnderlyingType()->ReferenceTo(), { ReferenceTo(), mod->GetIntegerType(bits, isSigned) });
    }

    llvm::Constant* ArrayType::CreateConstantValue(ArrayView<Constant*> value)
    {
        assert((int64_t)value.size() == Length() && "Value size differs from array length!");
        auto v = value | vs::transform([=](Constant* ptr) { return ptr->ConstantValue(); }) | to_vector;
        return llvm::ConstantArray::get(Instance(), v);
    }

    Constant* ArrayType::CreateConstant(vector<Constant*> value)
    {
        auto it = constantMap.find(value);
        if (it == constantMap.end())
        {
            auto c = CreateConstantValue(value);
            auto v = new llvm::GlobalVariable(*Module()->Module(), Instance(), true, llvm::GlobalValue::LinkageTypes::InternalLinkage, c);
            bool _;
            tie(it, _) = constantMap.emplace(move(value), Module()->CreateObject<Constant>(ReferenceTo(), v, c));
        }
        return it->second;
    }

    BooleanType::BooleanType(ModuleContext* mod) :
        ParentClass(mod, "bool", llvm::Type::getInt1Ty(mod->Context())),
        trueConstant(nullptr), falseConstant(nullptr)
    {

    }

    void BooleanType::DeclareMethods()
    {
        auto m = Module();
        auto vType = m->GetVoidType();
        auto refType = ReferenceTo();
        auto constructor = AddFunction(CONSTRUCTOR);
        constructor->AddIntrinsicOverload(vType, { refType }, false,
            [=, this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
            {
                caller->Builder()->CreateStore(CreateConstantValue(false), args[0]->Value());
                return nullptr;
            }
        );
        constructor->AddIntrinsicOverload(vType, { refType, refType }, false,
            [=, this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
            {
                auto builder = caller->Builder();
                auto v = builder->CreateLoad(args[1]->Type()->RemoveReference()->Instance(), args[1]->Value());
                builder->CreateStore(v, args[0]->Value());
                return nullptr;
            }
        );

        AddFunction(DESTRUCTOR)->AddIntrinsicOverload(vType, { refType }, false, IntrinsicFunction::Empty);

        FindFunction(ASSIGN_OPERATOR)->AddIntrinsicOverload(refType, { refType, refType }, false,
            [this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
            {
                auto builder = caller->Builder();
                auto lhs = args[0];
                auto rhs = args[1];
                auto v = builder->CreateLoad(rhs->Type()->RemoveReference()->Instance(), rhs->Value());
                builder->CreateStore(v, lhs->Value());
                return lhs;
            }
        );

        FindFunction(AND_OPERATOR)->AddIntrinsicOverload(this, { this, this }, false, CREATE_BINARY_OPERATOR(CreateAnd));
        FindFunction(OR_OPERATOR)->AddIntrinsicOverload(this, { this, this }, false, CREATE_BINARY_OPERATOR(CreateOr));
        FindFunction(EQUAL_OPERATOR)->AddIntrinsicOverload(this, { this, this }, false, CREATE_BINARY_OPERATOR(CreateICmpEQ));
        FindFunction(NOT_EQUAL_OPERATOR)->AddIntrinsicOverload(this, { this, this }, false, CREATE_BINARY_OPERATOR(CreateXor));

        FindFunction(AMPERSAND_OPERATOR)->AddIntrinsicOverload(this, { this, this }, false, CREATE_BINARY_OPERATOR(CreateAnd));
        FindFunction(PIPE_OPERATOR)->AddIntrinsicOverload(this, { this, this }, false, CREATE_BINARY_OPERATOR(CreateOr));

        FindFunction(NOT_OPERATOR)->AddIntrinsicOverload(this, { this }, false,
            [this](FunctionOverload* caller, ArrayView<Expression*> args, Expression* inst) -> Expression*
            {
                auto builder = caller->Builder();
                auto v = builder->CreateLoad(args[0]->Type()->RemoveReference()->Instance(), args[0]->Value());
                builder->CreateStore(builder->CreateNeg(v), inst->Value());
                return inst;
            }
        );

        Type::DeclareMethods();
    }

    llvm::ConstantInt* BooleanType::CreateConstantValue(bool value)
    {
        auto& ctx = Module()->Context();
        return value ? llvm::ConstantInt::getTrue(ctx) : llvm::ConstantInt::getFalse(ctx);
    }

    Constant* BooleanType::CreateConstant(bool value)
    {
        auto& it = value ? trueConstant : falseConstant;
        if (!it)
        {
            auto name = value ? "true" : "false";
            auto c = CreateConstantValue(value);
            auto v = new llvm::GlobalVariable(*Module()->Module(), Instance(), true, llvm::GlobalValue::LinkageTypes::InternalLinkage, c, name);
            it = Module()->CreateObject<Constant>(ReferenceTo(), v, c);
        }
        return it;
    }

    CharacterType::CharacterType(ModuleContext* mod) : ParentClass(mod, "char", llvm::Type::getInt8Ty(mod->Context())), constants{  }
    {

    }

    llvm::Constant* CharacterType::CreateConstantValue(char value)
    {
        return llvm::ConstantInt::get(Instance(), value, std::is_signed_v<char>);
    }

    Constant* CharacterType::CreateConstant(char value)
    {
        auto& result = constants[(unsigned char)value];
        if (!result)
        {
            auto name = (unsigned)value < 127 && std::isalnum(value) ? "const_char_{}"_f(value) : "const_char_0x{:X}"_f((uint8_t)value);
            auto c = CreateConstantValue(value);
            auto v = new llvm::GlobalVariable(*Module()->Module(), Instance(), true, llvm::GlobalValue::LinkageTypes::InternalLinkage, c, name);
            result = Module()->CreateObject<Constant>(ReferenceTo(), v, c);
        }
        return result;
    }

    void CharacterType::DeclareMethods()
    {

    }

    ClassType::ClassType(ModuleContext* mod, string name) : ParentClass(mod, move(name), llvm::StructType::create(mod->Context(), name))
    {

    }

    llvm::StructType* ClassType::Instance() const
    {
        return static_cast<llvm::StructType*>(Type::Instance());
    }

    void ClassType::DeclareMethods()
    {
        Type::DeclareMethods();
    }

    ClassType& ClassType::BeginFields()
    {
        assert(!fieldsDefined && "Type fields are already defined!");
#ifdef _DEBUG
        fieldsDefined = true;
#endif
        return *this;
    }

    ClassType& ClassType::DeclareField(string name, Type* type)
    {
        AddField(move(name), type);
        return *this;
    }

    ClassType& ClassType::EndFields()
    {
        assert(fieldsDefined && "Use BeginFields first!");
        if (auto f = Fields(); !f.empty())
            Instance()->setBody(f | vs::transform([](auto f) { return f->Type->Instance(); }) | to_vector);
        else Instance()->setBody({ llvm::IntegerType::get(Module()->Context(), 32) });
        return *this;
    }

    ClassType& ClassType::Begin()
    {
        assert(fieldsDefined && "Type must define it's fields first!");
        assert(!defined && "Type is already defined!");
#ifdef _DEBUG
        defined = true;
#endif
        return *this;
    }

    FunctionOverload* ClassType::DeclareConstructor(vector<ArgumentDeclaration> params)
    {
        return DeclareMethod(CONSTRUCTOR, move(params), Module()->GetVoidType());
    }

    FunctionOverload* ClassType::DeclareDestructor()
    {
        return DeclareMethod(DESTRUCTOR, {}, Module()->GetVoidType());
    }

    FunctionOverload* ClassType::DeclareMethod(string_view name, vector<ArgumentDeclaration> params, Type* returnType, MethodOptions options)
    {
        auto fn = FindFunction(name);
        if (!fn) fn = AddFunction(string(name));
        params.emplace(params.begin(), "this", ReferenceTo());
        return fn->AddOverload(returnType, move(params), options.IsVariadic, options.NoMangling);
    }

    void ClassType::End()
    {
        assert(defined && "Use Begin first!");
        DeclareMethods();
    }

    FloatType::FloatType(ModuleContext* mod, int bits) :
        ParentClass
        (
            mod,
            "f{}"_f(bits),
            bits == 32 ? llvm::Type::getFloatTy(mod->Context()) : llvm::Type::getDoubleTy(mod->Context())
        ),
        bits(bits)
    {
        assert(bits == 64 || bits == 32);
    }

    void FloatType::DeclareMethods()
    {
        auto m = Module();
        auto refType = ReferenceTo();
        auto vType = m->GetVoidType();
        auto bType = m->GetBooleanType();
        auto constructor = AddFunction(CONSTRUCTOR);
        constructor->AddIntrinsicOverload(vType, { refType }, false,
            [=, this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
            {
                auto builder = caller->Builder();
                builder->CreateStore(CreateConstantValue("0"), args[0]->Value());
                return nullptr;
            }
        );
        constructor->AddIntrinsicOverload(vType, { refType, refType }, false,
            [=, this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
            {
                auto builder = caller->Builder();
                auto v = builder->CreateLoad(args[1]->Type()->RemoveReference()->Instance(), args[1]->Value());
                builder->CreateStore(v, args[0]->Value());
                return nullptr;
            }
        );

        for (auto isSigned : { true, false })
            for (auto bits : { 8, 16, 32, 64 })
            {
                auto other = m->GetIntegerType(bits, isSigned);
                constructor->AddIntrinsicOverload(vType, { refType, other }, false,
                    [=, this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
                    {
                        auto builder = caller->Builder();
                        auto v = builder->CreateLoad(args[1]->Value()->getType(), args[1]->Value());
                        builder->CreateStore(isSigned ? builder->CreateSIToFP(v, Instance()) : builder->CreateUIToFP(v, Instance()), args[0]->Value());
                        return nullptr;
                    }
                );
            }

        AddFunction(DESTRUCTOR)->AddIntrinsicOverload(vType, { refType }, false, IntrinsicFunction::Empty);

        FindFunction(ASSIGN_OPERATOR)->AddIntrinsicOverload(refType, { refType, refType }, false,
            [this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
            {
                auto builder = caller->Builder();
                auto lhs = args[0];
                auto rhs = args[1];
                auto v = builder->CreateLoad(rhs->Type()->RemoveReference()->Instance(), rhs->Value());
                builder->CreateStore(v, lhs->Value());
                return lhs;
            }
        );

        auto inc = FindFunction(INCREMENT_OPERATOR)->AddIntrinsicOverload(refType, { refType }, false,
            [this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
            {
                auto inst = args[0];
                auto builder = caller->Builder();
                auto v1 = (llvm::Value*)builder->CreateLoad(inst->Type()->RemoveReference()->Instance(), inst->Value());
                auto v2 = CreateConstantValue("1");
                auto v = builder->CreateFAdd(v1, v2);
                builder->CreateStore(v, inst->Value());
                return inst;
            }
        );

        auto dec = FindFunction(DECREMENT_OPERATOR)->AddIntrinsicOverload(refType, { refType }, false,
            [this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
            {
                auto inst = args[0];
                auto builder = caller->Builder();
                auto v1 = (llvm::Value*)builder->CreateLoad(inst->Type()->RemoveReference()->Instance(), inst->Value());
                auto v2 = CreateConstantValue("1");
                auto v = builder->CreateFSub(v1, v2);
                builder->CreateStore(v, inst->Value());
                return inst;
            }
        );

        FindFunction(POST_INCREMENT_OPERATOR)->AddIntrinsicOverload(this, { refType }, false,
            [=, this](FunctionOverload* caller, ArrayView<Expression*> args, Expression* inst) -> Expression*
            {
                Construct(caller, inst, { args[0] });
                inc->Call(caller, args, nullptr);
                return inst;
            }
        );

        FindFunction(POST_DECREMENT_OPERATOR)->AddIntrinsicOverload(this, { refType }, false,
            [=, this](FunctionOverload* caller, ArrayView<Expression*> args, Expression* inst) -> Expression*
            {
                Construct(caller, inst, { args[0] });
                dec->Call(caller, args, nullptr);
                return inst;
            }
        );

        auto plus = FindFunction(PLUS_OPERATOR);
        plus->AddIntrinsicOverload(this, { this, this }, false, CREATE_BINARY_OPERATOR(CreateFAdd));
        plus->AddIntrinsicOverload(refType, { refType }, false,
            [this](FunctionOverload*, ArrayView<Expression*> args, Expression*) -> Expression*
            {
                return args[0];
            }
        );
        auto minus = FindFunction(MINUS_OPERATOR);
        minus->AddIntrinsicOverload(this, { this, this }, false, CREATE_BINARY_OPERATOR(CreateFSub));
        minus->AddIntrinsicOverload(this, { this }, false,
            [=](FunctionOverload* caller, ArrayView<Expression*> args, Expression* inst)->Expression*
            {
                auto builder = caller->Builder();
                auto v = builder->CreateLoad(args[0]->Type()->RemoveReference()->Instance(), args[0]->Value());
                auto result = builder->CreateFNeg(v);
                builder->CreateStore(result, inst->Value());
                return args[0];
            }
        );

        FindFunction(STAR_OPERATOR)->AddIntrinsicOverload(this, { this, this }, false, CREATE_BINARY_OPERATOR(CreateFMul));
        FindFunction(SLASH_OPERATOR)->AddIntrinsicOverload(this, { this, this }, false, CREATE_BINARY_OPERATOR(CreateFDiv));

        FindFunction(PLUS_ASSIGN_OPERATOR)->AddIntrinsicOverload(refType, { refType, this }, false, CREATE_ASSIGNMENT_OPERATOR(CreateFAdd));
        FindFunction(MINUS_ASSIGN_OPERATOR)->AddIntrinsicOverload(refType, { refType, this }, false, CREATE_ASSIGNMENT_OPERATOR(CreateFSub));
        FindFunction(STAR_ASSIGN_OPERATOR)->AddIntrinsicOverload(refType, { refType, this }, false, CREATE_ASSIGNMENT_OPERATOR(CreateFMul));
        FindFunction(SLASH_ASSIGN_OPERATOR)->AddIntrinsicOverload(refType, { refType, this }, false, CREATE_ASSIGNMENT_OPERATOR(CreateFDiv));

        //Comparison
        FindFunction(EQUAL_OPERATOR)->AddIntrinsicOverload(bType, { this, this }, false, CREATE_BINARY_OPERATOR(CreateFCmpOEQ));
        FindFunction(NOT_EQUAL_OPERATOR)->AddIntrinsicOverload(bType, { this, this }, false, CREATE_BINARY_OPERATOR(CreateFCmpUNE));
        FindFunction(LESS_OPERATOR)->AddIntrinsicOverload(bType, { this, this }, false, CREATE_BINARY_OPERATOR(CreateFCmpOLT));
        FindFunction(GREATER_OPERATOR)->AddIntrinsicOverload(bType, { this, this }, false, CREATE_BINARY_OPERATOR(CreateFCmpOGT));
        FindFunction(LESS_EQUAL_OPERATOR)->AddIntrinsicOverload(bType, { this, this }, false, CREATE_BINARY_OPERATOR(CreateFCmpOLE));
        FindFunction(GREATER_EQUAL_OPERATOR)->AddIntrinsicOverload(bType, { this, this }, false, CREATE_BINARY_OPERATOR(CreateFCmpOGE));

        Type::DeclareMethods();
    }

    llvm::Constant* FloatType::CreateConstantValue(llvm::StringRef value)
    {
        return llvm::ConstantFP::get(Instance(), value);
    }

    llvm::Constant* FloatType::CreateConstantValue(float value)
    {
        return CreateConstantValue(std::to_string(value));
    }

    llvm::Constant* FloatType::CreateConstantValue(double value)
    {
        return llvm::ConstantFP::get(Instance(), value);
    }

    Constant* FloatType::CreateConstant(float value)
    {
        auto id = std::to_string(value);
        auto it = constantMap.find(id);
        if (it == constantMap.end())
        {
            auto name = "const_f{}_{}"_f(bits, id);
            auto c = CreateConstantValue(value);
            auto v = new llvm::GlobalVariable(*Module()->Module(), Instance(), true, llvm::GlobalValue::LinkageTypes::InternalLinkage, c, name);
            bool _;
            tie(it, _) = constantMap.emplace(move(id), Module()->CreateObject<Constant>(ReferenceTo(), v, c));
        }
        return it->second;
    }

    FunctionPointerType::FunctionPointerType(ModuleContext* mod, Type* returnType, vector<Type*> parameterTypes, bool isVariadic) :
        ParentClass
        (
            mod,
            "({}{})->{}"_f(join(parameterTypes | name_of_range, ", "), isVariadic ? (parameterTypes.empty() ? "..." : ", ...") : "", returnType->Name()),
            llvm::FunctionType::get
            (
                returnType->TypeOfReturn(),
                [&]
                {
                    vector<llvm::Type*> v;
                    if (returnType->IsStructReturn())
                        v.emplace_back(returnType->ReferenceTo()->Instance());
                    for (auto ptr : parameterTypes)
                        v.emplace_back(ptr->Instance());
                    return v;
                }(),
                isVariadic
            )
        ),
        returnType(returnType),
        isVariadic(isVariadic)
    {
        this->parameterTypes = move(parameterTypes);
    }

    void FunctionPointerType::DeclareMethods()
    {

    }

    IntegerType::IntegerType(ModuleContext* mod, int bits, bool isSigned) :
        ParentClass(mod, "{}{}"_f(isSigned ? 'i' : 'u', bits), llvm::IntegerType::get(mod->Context(), bits)),
        bits(bits), isSigned(isSigned)
    {
        assert(bits == 128 || bits == 64 || bits == 32 || bits == 16 || bits == 8 && "Invalid bit size!");
    }

    void IntegerType::DeclareMethods()
    {
        auto m = Module();
        auto refType = ReferenceTo();
        auto vType = m->GetVoidType();
        auto bType = m->GetBooleanType();
        auto constructor = AddFunction(CONSTRUCTOR);
        constructor->AddIntrinsicOverload(vType, { refType }, false,
            [=, this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
            {
                auto builder = caller->Builder();
                builder->CreateStore(CreateConstantValue((int64_t)0), args[0]->Value());
                return nullptr;
            }
        );
        constructor->AddIntrinsicOverload(vType, { refType, refType }, false,
            [=, this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
            {
                auto builder = caller->Builder();
                auto v = builder->CreateLoad(args[1]->Type()->RemoveReference()->Instance(), args[1]->Value());
                builder->CreateStore(v, args[0]->Value());
                return nullptr;
            }
        );

        for (auto isSigned : { true, false })
            for (auto bits : { 8, 16, 32, 64 })
            {
                auto other = m->GetIntegerType(bits, isSigned);
                if (other == this)
                    continue;
                constructor->AddIntrinsicOverload(vType, { refType, other }, false,
                    [=, this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
                    {
                        auto builder = caller->Builder();
                        auto v = builder->CreateLoad(args[1]->Type()->RemoveReference()->Instance(), args[1]->Value());
                        builder->CreateStore(builder->CreateSExt(v, Instance()), args[0]->Value());
                        return nullptr;
                    }
                );
            }

        for (auto other : { m->GetFloatType(), m->GetDoublingType() })
            constructor->AddIntrinsicOverload(vType, { refType, other }, false,
                [=, this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
                {
                    auto builder = caller->Builder();
                    auto v = builder->CreateLoad(args[1]->Type()->RemoveReference()->Instance(), args[1]->Value());
                    builder->CreateStore(IsSigned() ? builder->CreateFPToSI(v, Instance()) : builder->CreateFPToUI(v, Instance()), args[0]->Value());
                    return nullptr;
                }
            );

        AddFunction(DESTRUCTOR)->AddIntrinsicOverload(vType, { refType }, false, IntrinsicFunction::Empty);

        FindFunction(ASSIGN_OPERATOR)->AddIntrinsicOverload(refType, { refType, refType }, false,
            [this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
            {
                auto builder = caller->Builder();
                auto lhs = args[0];
                auto rhs = args[1];
                auto v = builder->CreateLoad(rhs->Type()->RemoveReference()->Instance(), rhs->Value());
                builder->CreateStore(v, lhs->Value());
                return lhs;
            }
        );

        auto inc = FindFunction(INCREMENT_OPERATOR)->AddIntrinsicOverload(refType, { refType }, false,
            [this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
            {
                auto inst = args[0];
                auto builder = caller->Builder();
                auto v1 = (llvm::Value*)builder->CreateLoad(inst->Type()->RemoveReference()->Instance(), inst->Value());
                auto v2 = llvm::ConstantInt::get(Instance(), 1, IsSigned());
                auto v = builder->CreateAdd(v1, v2);
                builder->CreateStore(v, inst->Value());
                return inst;
            }
        );

        auto dec = FindFunction(DECREMENT_OPERATOR)->AddIntrinsicOverload(refType, { refType }, false,
            [this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
            {
                auto inst = args[0];
                auto builder = caller->Builder();
                auto v1 = (llvm::Value*)builder->CreateLoad(inst->Type()->RemoveReference()->Instance(), inst->Value());
                auto v2 = llvm::ConstantInt::get(Instance(), 1, IsSigned());
                auto v = builder->CreateSub(v1, v2);
                builder->CreateStore(v, inst->Value());
                return inst;
            }
        );

        FindFunction(POST_INCREMENT_OPERATOR)->AddIntrinsicOverload(this, { refType }, false,
            [=, this](FunctionOverload* caller, ArrayView<Expression*> args, Expression* inst) -> Expression*
            {
                Construct(caller, inst, { args[0] });
                inc->Call(caller, args, nullptr);
                return inst;
            }
        );

        FindFunction(POST_DECREMENT_OPERATOR)->AddIntrinsicOverload(this, { refType }, false,
            [=, this](FunctionOverload* caller, ArrayView<Expression*> args, Expression* inst) -> Expression*
            {
                Construct(caller, inst, { args[0] });
                dec->Call(caller, args, nullptr);
                return inst;
            }
        );

        auto plus = FindFunction(PLUS_OPERATOR);
        plus->AddIntrinsicOverload(this, { this, this }, false, CREATE_BINARY_OPERATOR(CreateAdd));
        plus->AddIntrinsicOverload(refType, { refType }, false,
            [this](FunctionOverload*, ArrayView<Expression*> args, Expression*) -> Expression*
            {
                return args[0];
            }
        );
        auto minus = FindFunction(MINUS_OPERATOR);
        minus->AddIntrinsicOverload(this, { this, this }, false, CREATE_BINARY_OPERATOR(CreateSub));
        if (IsSigned())
            minus->AddIntrinsicOverload(this, { this }, false,
                [=](FunctionOverload* caller, ArrayView<Expression*> args, Expression* inst)->Expression*
                {
                    auto builder = caller->Builder();
                    auto v = builder->CreateLoad(args[0]->Type()->RemoveReference()->Instance(), args[0]->Value());
                    auto result = builder->CreateNeg(v);
                    builder->CreateStore(result, inst->Value());
                    return args[0];
                }
            );

        FindFunction(STAR_OPERATOR)->AddIntrinsicOverload(this, { this, this }, false, CREATE_BINARY_OPERATOR(CreateMul));
        FindFunction(SLASH_OPERATOR)->AddIntrinsicOverload(this, { this, this }, false, CREATE_BINARY_OPERATOR_SU(CreateSDiv, CreateUDiv));
        FindFunction(MODULO_OPERATOR)->AddIntrinsicOverload(this, { this, this }, false, CREATE_BINARY_OPERATOR_SU(CreateSRem, CreateURem));

        FindFunction(PLUS_ASSIGN_OPERATOR)->AddIntrinsicOverload(refType, { refType, this }, false, CREATE_ASSIGNMENT_OPERATOR(CreateAdd));
        FindFunction(MINUS_ASSIGN_OPERATOR)->AddIntrinsicOverload(refType, { refType, this }, false, CREATE_ASSIGNMENT_OPERATOR(CreateSub));
        FindFunction(STAR_ASSIGN_OPERATOR)->AddIntrinsicOverload(refType, { refType, this }, false, CREATE_ASSIGNMENT_OPERATOR(CreateMul));
        FindFunction(SLASH_ASSIGN_OPERATOR)->AddIntrinsicOverload(refType, { refType, this }, false, CREATE_ASSIGNMENT_OPERATOR_SU(CreateSDiv, CreateUDiv));
        FindFunction(MODULO_ASSIGN_OPERATOR)->AddIntrinsicOverload(refType, { refType, this }, false, CREATE_ASSIGNMENT_OPERATOR_SU(CreateSRem, CreateURem));

        //Bitwise
        FindFunction(TILDE_OPERATOR)->AddIntrinsicOverload(this, { this }, false,
            [=, this](FunctionOverload* caller, ArrayView<Expression*> args, Expression* inst) -> Expression*
            {
                auto builder = caller->Builder();
                auto v = builder->CreateLoad(args[0]->Type()->RemoveReference()->Instance(), args[0]->Value());
                auto result = builder->CreateNot(v);
                builder->CreateStore(result, inst->Value());
                return args[0];
            }
        );

        FindFunction(PIPE_OPERATOR)->AddIntrinsicOverload(this, { this, this }, false, CREATE_BINARY_OPERATOR(CreateOr));
        FindFunction(AMPERSAND_OPERATOR)->AddIntrinsicOverload(this, { this, this }, false, CREATE_BINARY_OPERATOR(CreateAnd));
        FindFunction(CARET_OPERATOR)->AddIntrinsicOverload(this, { this, this }, false, CREATE_BINARY_OPERATOR(CreateXor));
        FindFunction(PIPE_ASSIGN_OPERATOR)->AddIntrinsicOverload(refType, { refType, this }, false, CREATE_ASSIGNMENT_OPERATOR(CreateOr));
        FindFunction(AMPERSAND_ASSIGN_OPERATOR)->AddIntrinsicOverload(refType, { refType, this }, false, CREATE_ASSIGNMENT_OPERATOR(CreateAnd));
        FindFunction(CARET_ASSIGN_OPERATOR)->AddIntrinsicOverload(refType, { refType, this }, false, CREATE_ASSIGNMENT_OPERATOR(CreateXor));

        //Shifts
        FindFunction(LEFT_SHIFT_OPERATOR)->AddIntrinsicOverload(this, { this, this }, false, CREATE_BINARY_OPERATOR(CreateShl));
        FindFunction(RIGHT_SHIFT_OPERATOR)->AddIntrinsicOverload(this, { this, this }, false, CREATE_BINARY_OPERATOR(CreateAShr));
        FindFunction(LEFT_SHIFT_ASSIGN_OPERATOR)->AddIntrinsicOverload(refType, { refType, this }, false, CREATE_ASSIGNMENT_OPERATOR(CreateShl));
        FindFunction(RIGHT_SHIFT_ASSIGN_OPERATOR)->AddIntrinsicOverload(refType, { refType, this }, false, CREATE_ASSIGNMENT_OPERATOR(CreateAShr));

        //Comparison
        FindFunction(EQUAL_OPERATOR)->AddIntrinsicOverload(bType, { this, this }, false, CREATE_BINARY_OPERATOR(CreateICmpEQ));
        FindFunction(NOT_EQUAL_OPERATOR)->AddIntrinsicOverload(bType, { this, this }, false, CREATE_BINARY_OPERATOR(CreateICmpNE));
        FindFunction(LESS_OPERATOR)->AddIntrinsicOverload(bType, { this, this }, false, CREATE_BINARY_OPERATOR_SU(CreateICmpSLT, CreateICmpULT));
        FindFunction(GREATER_OPERATOR)->AddIntrinsicOverload(bType, { this, this }, false, CREATE_BINARY_OPERATOR_SU(CreateICmpSGT, CreateICmpUGT));
        FindFunction(LESS_EQUAL_OPERATOR)->AddIntrinsicOverload(bType, { this, this }, false, CREATE_BINARY_OPERATOR_SU(CreateICmpSLE, CreateICmpULE));
        FindFunction(GREATER_EQUAL_OPERATOR)->AddIntrinsicOverload(bType, { this, this }, false, CREATE_BINARY_OPERATOR_SU(CreateICmpSGE, CreateICmpUGE));

        Type::DeclareMethods();
    }

    llvm::ConstantInt* IntegerType::CreateConstantValueUnsigned(uint64_t value)
    {
        return llvm::ConstantInt::get(Instance(), value, IsSigned());
    }

    llvm::ConstantInt* IntegerType::CreateConstantValue(int64_t value)
    {
        return CreateConstantValueUnsigned((uint64_t)value);
    }

    Constant* IntegerType::CreateConstantUnsigned(uint64_t value)
    {
        auto it = constantMap.find(value);
        if (it == constantMap.end())
        {
            auto name = "const_{}_{}"_f(Name(), value);
            auto c = CreateConstantValueUnsigned(value);
            auto v = new llvm::GlobalVariable(*Module()->Module(), Instance(), true, llvm::GlobalValue::LinkageTypes::InternalLinkage, c, name);
            bool _;
            tie(it, _) = constantMap.emplace(value, Module()->CreateObject<Constant>(ReferenceTo(), v, c));
        }
        return it->second;
    }

    Constant* IntegerType::CreateConstant(int64_t value)
    {
        return CreateConstantUnsigned((uint64_t)value);
    }

    Expression* IntegerType::PassArgument(FunctionOverload* fn, Expression* expr)
    {
        assert(expr->Type()->RemoveReference() == this);
        if (auto c = expr->As<Constant>())
        {
            for (auto& [k, v] : constantMap) if (v == c)
                return Module()->CreateObject<Expression>(this, CreateConstantValueUnsigned(k));
            throw std::runtime_error("Invalid constant expression!");
        }
        auto v = fn->Builder()->CreateLoad(Instance(), expr->Value());
        return Module()->CreateObject<Expression>(this, v);
    }

    uint64_t IntegerType::GetConstantValue(Constant* c)
    {
        for (auto& [k, v] : constantMap) if (v == c)
            return k;
        throw std::runtime_error("Invalid constant expression!");
    }

    OptionalType::OptionalType(ModuleContext* mod, Type* underlyingType) :
        ParentClass
        (
            mod,
            "{}?"_f(underlyingType->Name()),
                llvm::StructType::get(mod->Context(), { mod->GetBooleanType()->Instance(), underlyingType->Instance() })
        ),
        underlyingType(underlyingType)
    {
        assert(underlyingType->IsNot<ReferenceType>() && "Optional reference is not valid!");
        assert(underlyingType->IsNot<OptionalType>() && "Optional optional is not valid!");
        assert(underlyingType->IsNot<VoidType>() && "Optional void is not valid!");
        AddField("0", mod->GetBooleanType());
        AddField("1", underlyingType);
    }

    void OptionalType::DeclareMethods()
    {
        auto m = Module();
        auto vType = m->GetVoidType();
        auto bType = m->GetBooleanType();
        auto refType = ReferenceTo();
        auto constructor = AddFunction(CONSTRUCTOR);
        {
            auto fn = constructor->AddOverload(vType, { {.Type = refType } });
            fn->Begin();
            bType->Construct(fn, fn->GetField(fn->Parameter(0), 0), { bType->CreateConstant(false) });
            fn->End();
        }
        {
            auto fn = constructor->AddOverload(vType, { {.Type = refType }, {.Type = underlyingType->ReferenceTo() } });
            fn->Begin();
            fn->Construct(fn->GetField(fn->Parameter(0), 0), { bType->CreateConstant(true) });
            fn->Construct(fn->GetField(fn->Parameter(0), 1), { fn->Parameter(1) });
            fn->End();
        }
        {
            auto fn = constructor->AddOverload(vType, { {.Type = refType }, {.Type = refType } });
            fn->Begin();
            auto [lhs, rhs] = fn->UnpackParameters<2>();
            auto v = fn->GetField(rhs, 0);
            fn->Construct(fn->GetField(lhs, 0), { v });
            fn->BeginIf();
            fn->AddIfCondition(v);
            fn->Construct(fn->GetField(lhs, 1), { fn->GetField(rhs, 1) });
            fn->EndIf();
            fn->End();
        }
        {
            auto fn = AddFunction(DESTRUCTOR)->AddOverload(vType, { {.Type = refType } });
            fn->Begin();
            fn->BeginIf();
            fn->AddIfCondition(fn->GetField(fn->Parameter(0), 0));
            fn->Destruct(fn->GetField(fn->Parameter(0), 1));
            fn->EndIf();
            fn->End();
        }
        {
            auto fn = FindFunction(ASSIGN_OPERATOR)->AddOverload(vType, { {.Type = refType }, {.Type = refType } });
            fn->Begin();
            auto [lhs, rhs] = fn->UnpackParameters<2>();
            fn->BeginIf();
            fn->AddIfCondition(fn->GetField(lhs, 0));
            fn->BeginCall(ASSIGN_OPERATOR);
            fn->AddCallParameter(fn->GetField(lhs, 1));
            fn->AddCallParameter(fn->GetField(rhs, 1));
            fn->EndCall();
            fn->BeginElse();
            fn->Construct(lhs, { rhs });
            fn->EndIf();
            fn->End();
        }
        Type::DeclareMethods();
    }

    PointerType::PointerType(ModuleContext* mod, Type* underlyingType) :
        ParentClass
        (
            mod,
            "{}*"_f(underlyingType->Name()),
            underlyingType->Is<VoidType>() ? llvm::Type::getInt8Ty(mod->Context())->getPointerTo() : underlyingType->Instance()->getPointerTo()
        ),
        underlyingType(underlyingType)
    {
        assert(underlyingType->IsNot<ReferenceType>() && "Pointer to reference is not valid!");
    }

    llvm::PointerType* PointerType::Instance() const
    {
        return static_cast<llvm::PointerType*>(Type::Instance());
    }

    void PointerType::DeclareMethods()
    {
        auto m = Module();
        auto voidTy = m->GetVoidType();
        auto refTy = ReferenceTo();
        auto boolTy = m->GetBooleanType();
        auto constructor = AddFunction(CONSTRUCTOR);
        constructor->AddIntrinsicOverload(voidTy, { refTy }, false,
            [=, this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
            {
                auto builder = caller->Builder();
                builder->CreateStore(llvm::ConstantPointerNull::get(Instance()), args[0]->Value());
                return nullptr;
            }
        );
        constructor->AddIntrinsicOverload(voidTy, { refTy, refTy }, false,
            [=, this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
            {
                auto builder = caller->Builder();
                auto v = builder->CreateLoad(args[1]->Type()->RemoveReference()->Instance(), args[1]->Value());
                builder->CreateStore(v, args[0]->Value());
                return nullptr;
            }
        );

        FindFunction(ASSIGN_OPERATOR)->AddIntrinsicOverload(refTy, { refTy, refTy }, false,
            [=, this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
            {
                auto builder = caller->Builder();
                auto v = builder->CreateLoad(args[1]->Type()->RemoveReference()->Instance(), args[1]->Value());
                builder->CreateStore(v, args[0]->Value());
                return args[0];
            }
        );

        AddFunction(DESTRUCTOR)->AddIntrinsicOverload(voidTy, { refTy }, false, IntrinsicFunction::Empty);

        if (underlyingType->IsNot<VoidType>())
        {
            auto vPtrTy = voidTy->PointerTo();
            vPtrTy->FindFunction(CONSTRUCTOR)->AddIntrinsicOverload(voidTy, { vPtrTy->ReferenceTo(), refTy }, false,
                [=, this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
                {
                    auto builder = caller->Builder();
                    auto v = builder->CreateLoad(args[1]->Type()->RemoveReference()->Instance(), args[1]->Value());
                    builder->CreateStore(builder->CreatePointerCast(v, voidTy->PointerTo()->Instance()), args[0]->Value());
                    return nullptr;
                }
            );

            vPtrTy->FindFunction(ASSIGN_OPERATOR)->AddIntrinsicOverload(refTy, { vPtrTy->ReferenceTo(), refTy }, false,
                [=, this](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
                {
                    auto builder = caller->Builder();
                    auto v = builder->CreateLoad(args[1]->Type()->RemoveReference()->Instance(), args[1]->Value());
                    builder->CreateStore(v, args[0]->Value());
                    return args[0];
                }
            );

            auto uRefType = UnderlyingType()->ReferenceTo();
            FindFunction(STAR_OPERATOR)->AddIntrinsicOverload(uRefType, { refTy }, false,
                [=](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
                {
                    auto builder = caller->Builder();
                    auto v = builder->CreateLoad(args[0]->Type()->RemoveReference()->Instance(), args[0]->Value());
                    return m->CreateObject<Expression>(uRefType, v);
                }
            );

        }

        auto i64 = m->GetIntegerType(64, true);

        for (auto isSigned : { true, false })
        for (auto bits : { 8, 16, 32, 64 })
        {
            auto iType = m->GetIntegerType(bits, isSigned);
            auto f = [=](FunctionOverload* caller, Expression* ptr, Expression* delta, Expression* inst) -> Expression*
            {
                auto builder = caller->Builder();
                llvm::Value* v = builder->CreateLoad(delta->Type()->RemoveReference()->Instance(), delta->Value());
                if (!isSigned || bits != 64)
                    v = builder->CreateSExt(v, i64->Instance());
                auto result = builder->CreateGEP(nullptr, ptr->Value(), v);
                builder->CreateStore(result, inst->Value());
                return inst;
            };
            FindFunction(PLUS_OPERATOR)->AddIntrinsicOverload(this, { this, iType }, false, [=](auto caller, auto args, auto inst) { return f(caller, args[0], args[1], inst); });
            FindFunction(PLUS_OPERATOR)->AddIntrinsicOverload(this, { iType, this }, false, [=](auto caller, auto args, auto inst) { return f(caller, args[1], args[0], inst); });

            FindFunction(PLUS_ASSIGN_OPERATOR)->AddIntrinsicOverload(this, { this, iType }, false, [=](auto caller, auto args, auto) { return f(caller, args[0], args[1], args[0]); });

            if (underlyingType->Is<VoidType>())
                continue;

            auto uRefType = UnderlyingType()->ReferenceTo();
            FindFunction(SUBSCRIPT_OPERATOR)->AddIntrinsicOverload(uRefType, { refTy, iType }, false,
                [=](FunctionOverload* caller, ArrayView<Expression*> args, Expression*) -> Expression*
                {
                    auto builder = caller->Builder();
                    llvm::Value* v = builder->CreateLoad(args[1]->Type()->RemoveReference()->Instance(), args[1]->Value());
                    if (!isSigned || bits != 64)
                        v = builder->CreateSExt(v, i64->Instance());
                    auto ptr = builder->CreateLoad(args[0]->Type()->RemoveReference()->Instance(), args[0]->Value());
                    v = builder->CreateGEP(uRefType->RemoveReference()->Instance(), ptr, v);
                    return m->CreateObject<Expression>(uRefType, v);
                }
            );

        }

        auto equalOp = FindFunction(EQUAL_OPERATOR);
        auto notEqualOp = FindFunction(NOT_EQUAL_OPERATOR);
        assert(equalOp && notEqualOp);

        equalOp->AddIntrinsicOverload(boolTy, { this, this }, false,
            [=](FunctionOverload* caller, ArrayView<Expression*> args, Expression* result) -> Expression*
            {
                auto builder = caller->Builder();
                auto v = builder->CreateICmpEQ(args[0]->Value(), args[1]->Value());
                builder->CreateStore(v, result->Value());
                return result;
            }
        );

        notEqualOp->AddIntrinsicOverload(boolTy, { this, this }, false,
            [=](FunctionOverload* caller, ArrayView<Expression*> args, Expression* result) -> Expression*
            {
                auto builder = caller->Builder();
                auto v = builder->CreateICmpNE(args[0]->Value(), args[1]->Value());
                builder->CreateStore(v, result->Value());
                return result;
            }
        );

        Type::DeclareMethods();
    }

    Type* PointerType::RemovePointer()
    {
        return UnderlyingType();
    }

    ReferenceType::ReferenceType(ModuleContext* mod, Type* underlyingType) :
        ParentClass(mod, "{}&"_f(underlyingType->Name()), underlyingType->Instance()->getPointerTo()),
        underlyingType(underlyingType)
    {
        assert(underlyingType->IsNot<VoidType>() && "Reference to void is not valid!");
        assert(underlyingType->IsNot<ReferenceType>() && "Reference to another reference is not valid!");
    }

    llvm::PointerType* ReferenceType::Instance() const
    {
        return static_cast<llvm::PointerType*>(Type::Instance());
    }

    Type* ReferenceType::RemoveReference()
    {
        return underlyingType;
    }

    Expression* ReferenceType::PassArgument(FunctionOverload* fn, Expression* expr)
    {
        return expr;
    }

    Expression* ReferenceType::Allocate(FunctionOverload* fn)
    {
        return nullptr;
    }

    void ReferenceType::DeclareMethods()
    {

    }

    void ReferenceType::Construct(FunctionOverload* fn, Expression* inst, vector<Expression*> args)
    {

    }

    TupleType::TupleType(ModuleContext* mod, vector<Type*> elementTypes) :
        ParentClass
        (
            mod,
            "({})"_f(join(elementTypes | name_of_range, ", ")),
            llvm::StructType::get(mod->Context(), elementTypes | vs::transform([](auto ty) { return ty->Instance(); }) | to_vector)
        ),
        elementTypes(move(elementTypes)),
        copyConstructor(nullptr), defConstructor(nullptr),
        isTriviallyCopyable(rn::all_of(elementTypes, [](Type* t) { return t->IsTriviallyCopyable(); }))
    {
        for (int i = 0; i < ElementCount(); ++i)
            AddField("${}"_f(i), ElementType(i));
    }

    void TupleType::DeclareMethods()
    {
        auto m = Module();
        auto voidTy = m->GetVoidType();
        {
            auto c = AddFunction(CONSTRUCTOR);
            auto paramTypes = vector<ArgumentDeclaration>{ { "this", ReferenceTo() } };
            auto types = ElementTypes();
            for (size_t i = 0; i < types.size(); ++i)
                paramTypes.emplace_back("_{}"_f(i + 1), types[i]);
            AddFunctionDeclaration(c, { paramTypes }, [this](FunctionOverload* fn)
            {
                auto tys = ElementTypes();
                auto inst = fn->Parameter(0);
                for (int i = 0; i < (int)tys.size(); ++i)
                {
                    fn->AddOption(GetField(fn, inst, i));
                    fn->BeginCall(tys[i]);
                    fn->AddCallParameter(fn->Parameter(i + 1));
                    fn->EndCall();
                }
            });
            AddFunctionDeclaration(c, { { { "this", ReferenceTo() }, { "rhs", ReferenceTo() } } }, [=](FunctionOverload* fn)
            {
                for (size_t i = 0; i < types.size(); ++i)
                    types[i]->Construct(fn, fn->GetField(fn->Parameter(0), i), { fn->GetField(fn->Parameter(1), i) });
            });
            if (rn::all_of(types, [](Type* ty) { return !ty->FindFunction(CONSTRUCTOR)->FindCallable({ ty->ReferenceTo() }).empty(); }))
                AddFunctionDeclaration(c, { { { "this", ReferenceTo() } } }, [=](FunctionOverload* fn)
                {
                    for (size_t i = 0; i < types.size(); ++i)
                        fn->Construct(fn->GetField(fn->Parameter(0), i), {  });
                });
        }
        Type::DeclareMethods();
    }

    VariantType::VariantType(ModuleContext* mod, vector<Type*> elementTypes) :
        ParentClass
        (
            mod,
            "({})"_f(join(elementTypes | name_of_range, " | ")),
            [&]
            {
                dominantType = reduce(elementTypes, [](auto l, auto r) { return l->Size() > r->Size() ? l : r; });
                vector<llvm::Type*> types{ mod->GetIntegerType(32, false)->Instance(), dominantType->Instance() };
                return llvm::StructType::get(mod->Context(), types);
            }()
        ),
        elementTypes(move(elementTypes)),
        copyConstructor(nullptr), defConstructor(nullptr),
        isTriviallyCopyable(rn::all_of(elementTypes, [](Type* t) { return t->IsTriviallyCopyable(); }))
    {
        AddField("0", mod->GetIntegerType(32, false));
        AddField("1", dominantType);
    }

    void VariantType::DeclareMethods()
    {
        auto m = Module();
        auto vType = m->GetVoidType();
        auto iType = m->GetIntegerType(32, false);
        {
            {
                auto c = AddFunction(CONSTRUCTOR);
                for (int64_t i = 0; auto ty : ElementTypes())
                {
                    auto fn = c->AddOverload(vType, { { "this", ReferenceTo() }, { "rhs", ty->ReferenceTo() } });
                    fn->Begin();
                    ty->Construct(fn, fn->GetField(fn->Parameter(0), 0), { iType->CreateConstant(i) });
                    ty->Construct(fn, GetTypeByIndex(fn, fn->Parameter(0), i), { fn->Parameter(1) });
                    fn->End();
                    ++i;
                }
                {
                    auto fn = c->AddOverload(vType, { { "this", ReferenceTo() }, { "rhs", ReferenceTo() } });
                    fn->Begin();
                    fn->BeginSwitch();
                    fn->AddSwitchCondition(fn->GetField(fn->Parameter(1), 0));
                    for (int64_t i = 0; auto _ : ElementTypes())
                    {
                        fn->BeginCase();
                        fn->AddCaseValue(iType->CreateConstant(i));
                        Construct(fn, fn->Parameter(0), { GetTypeByIndex(fn, fn->Parameter(1), i) });
                        fn->EndCase();
                        ++i;
                    }
                    fn->EndSwitch();
                    fn->End();
                }
            }
            {
                auto fn = AddFunction(DESTRUCTOR)->AddOverload(vType, { { "this", ReferenceTo() } });
                fn->Begin();
                fn->BeginSwitch();
                fn->AddSwitchCondition(fn->GetField(fn->Parameter(0), 0));
                for (int64_t i = 0; auto ty : ElementTypes())
                {
                    fn->BeginCase();
                    fn->AddCaseValue(iType->CreateConstant(i));
                    ty->Destruct(fn, GetTypeByIndex(fn, fn->Parameter(0), i));
                    fn->EndCase();
                    ++i;
                }
                fn->EndSwitch();
                fn->End();
            }
            {
                auto assign = FindFunction(ASSIGN_OPERATOR);
                for (auto i = 0; auto ty : ElementTypes())
                {
                    auto fn = assign->AddOverload(vType, { { "this", ReferenceTo() }, { "rhs", ty->ReferenceTo() } });
                    fn->Begin();
                    auto [lhs, rhs] = fn->UnpackParameters<2>();
                    Destruct(fn, lhs);
                    Construct(fn, lhs, { rhs });
                    fn->End();
                    ++i;
                }
                {
                    auto fn = assign->AddOverload(vType, { { "this", ReferenceTo() }, { "rhs", ReferenceTo() } });
                    fn->Begin();
                    auto [lhs, rhs] = fn->UnpackParameters<2>();
                    Destruct(fn, lhs);
                    Construct(fn, lhs, { rhs });
                    fn->End();
                }
            }
        }
    }

    Expression* VariantType::GetTypeByIndex(FunctionOverload* fn, Expression* inst, int64_t i)
    {
        inst = fn->GetField(inst, 1);
        if (auto ty = ElementType(i); inst->Type()->RemoveReference() != ty)
            inst = fn->PointerCast(inst, ty);
        return inst;
    }

    VoidType::VoidType(ModuleContext* mod) : ParentClass(mod, "void", llvm::Type::getVoidTy(mod->Context()))
    {

    }

    llvm::Type* VoidType::TypeOfParameter() const
    {
        return llvm::Type::getVoidTy(Module()->Context());
    }

    Expression* VoidType::Allocate(FunctionOverload*)
    {
        throw std::runtime_error("Allocated called on void type!");
    }

    void VoidType::DeclareMethods()
    {

    }

    void VoidType::Construct(FunctionOverload* fn, Expression* inst, vector<Expression*> args)
    {
        throw std::runtime_error("Construct called on void type!");
    }

}
