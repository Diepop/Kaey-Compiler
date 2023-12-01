#include "Kaey/Llvm/ModuleContext.hpp"
#include "Kaey/Llvm/JitCompiler.hpp"
#include "Kaey/Llvm/Types.hpp"
#include "Kaey/Llvm/Function.hpp"

#ifndef _DEBUG

static void PrintModule(llvm::Module* = nullptr) {  }

#else

void PrintModule(llvm::Module* mod = nullptr)
{
    static const auto Module = mod;
    system("cls\n");
    Module->dump();
}

#endif

//x64 double -> n >= 27, float -> n >= 29
//x86 double -> n >= 32, float -> n >= 33

using namespace Kaey::Llvm;
using namespace Kaey::Operators;

void DeclareIntFns(ModuleContext* ctx);
void DeclareFloatFns(ModuleContext* ctx);

int main(int argc, char* argv[])
{
    try
    {
        std::default_random_engine rd((unsigned int)std::chrono::high_resolution_clock::now().time_since_epoch().count());
        std::uniform_int_distribution dst(0, 100);
        srand(unsigned(137 * dst(rd) + 1337891));
        JitCompiler jit;
        auto mod = std::make_unique<llvm::Module>("Test", jit.Context());
        PrintModule(mod.get());
        auto ctx = std::make_unique<ModuleContext>(jit.Context(), mod.get(), &jit.DataLayout());

        auto i32 = ctx->GetIntegerType(32, true);
        auto i64 = ctx->GetIntegerType(64, true);
        auto f32 = ctx->GetFloatType();
        auto voidTy = ctx->GetVoidType();
        auto voidPtrTy = voidTy->PointerTo();
        auto charTy = ctx->GetCharacterType();
        auto boolTy = ctx->GetBooleanType();

        auto v2Ty = ctx->DeclareClass("Vector2");
        v2Ty->BeginFields()
            .DeclareField("x", f32)
            .DeclareField("y", f32)
        .EndFields()
        .Begin();
            v2Ty->DeclareMethod("Dot", { { "rhs", v2Ty->ReferenceTo() } }, f32);
        v2Ty->End();

        for (auto ty : ctx->Types())
            ty->CreateMethods();

        {
            auto fn = v2Ty->FindMethodOverload("Dot", { v2Ty->ReferenceTo() })->As<FunctionOverload>();
            fn->Begin();
                fn->BeginCall(PLUS_OPERATOR);
                    fn->BeginCall(STAR_OPERATOR);
                        fn->AddCallParameter(fn->Parameter(0)->GetField(fn, 0));
                        fn->AddCallParameter(fn->Parameter(1)->GetField(fn, 0));
                    fn->AddCallParameter(fn->EndCall());
                    fn->BeginCall(STAR_OPERATOR);
                        fn->AddCallParameter(fn->Parameter(0)->GetField(fn, 1));
                        fn->AddCallParameter(fn->Parameter(1)->GetField(fn, 1));
                    fn->AddCallParameter(fn->EndCall());
                fn->CreateReturn(fn->EndCall());
            fn->End();
        }

        auto putsFn = ctx->DeclareFunction("puts", { { .Type = charTy->PointerTo() } }, i32, false, true);
        auto printfFn = ctx->DeclareFunction("printf", { { .Type = charTy->PointerTo() } }, i32, true, true);
        auto scanfFn = ctx->DeclareFunction("scanf", { { .Type = charTy->PointerTo() } }, i32, true, true);
        auto randFn = ctx->DeclareFunction("rand", {  }, i32, false, true);
        auto mallocFn = ctx->DeclareFunction("malloc", { { .Type = i64 } }, voidPtrTy, false, true);
        auto freeFn = ctx->DeclareFunction("free", { { .Type = voidPtrTy } }, voidTy, false, true);

        auto randomFn = ctx->DeclareFunction("random", {  }, i32, false, true);
        {
            randomFn->Begin();
                randomFn->AddOption(ReturnCall{  });
                randomFn->BeginCall(ctx->FindFunction(MODULO_OPERATOR));
                    randomFn->AddCallParameter(randomFn->CallUnscoped(randFn));
                    randomFn->AddCallParameter(i32->CreateConstant(100));
                randomFn->EndCall();
            randomFn->End();
        }

        {
            auto f = ctx->DeclareFunction("IfTest", { { .Type = v2Ty->ReferenceTo() }, { .Type = v2Ty->ReferenceTo() } }, voidTy, false, false);
            f->Begin();
            auto [lhs, rhs] = f->UnpackParameters<2>();
            f->BeginIf();
                f->BeginCall(EQUAL_OPERATOR);
                    f->AddCallParameter(lhs);
                    f->AddCallParameter(rhs);
            f->AddIfCondition(f->EndCall());
                f->BeginCall(putsFn);
                    f->BeginCall(PLUS_OPERATOR);
                        f->AddCallParameter(ctx->CreateStringConstant("Vectors are equal!"));
                    f->AddCallParameter(f->EndCall());
                f->EndCall();
            f->BeginElse();
                f->BeginCall(putsFn);
                    f->BeginCall(PLUS_OPERATOR);
                        f->AddCallParameter(ctx->CreateStringConstant("Vectors not are equal!"));
                    f->AddCallParameter(f->EndCall());
                f->EndCall();
            f->EndIf();
            f->End();
        }

        auto ifTest = ctx->DeclareFunction("IfTest", { { .Type = i32 } }, voidTy, false, false);
        {
            ifTest->Begin();
            {
                ifTest->BeginIf();
                    ifTest->BeginCall(EQUAL_OPERATOR);
                        ifTest->AddCallParameter(ifTest->Parameter(0));
                        ifTest->AddCallParameter(i32->CreateConstant(0));
                    ifTest->AddIfCondition(ifTest->EndCall());
                ifTest->CallUnscoped(randomFn);
                ifTest->BeginElse();
                    ifTest->BeginCall(STAR_OPERATOR);
                        ifTest->AddCallParameter(ifTest->CallUnscoped(randomFn));
                        ifTest->AddCallParameter(i32->CreateConstant(5));
                    ifTest->EndCall();
                ifTest->EndIf();
            }
            ifTest->End();
            PrintModule();
        }

        {
            auto fn = ctx->DeclareFunction("LoopTest", { { "count", i32 } }, i32, false, true);
            fn->Begin();
            {
                fn->AddOption("i");
                auto i = fn->CallUnscoped(i32);
                fn->BeginWhile();
                    fn->BeginCall(EQUAL_OPERATOR);
                        fn->AddCallParameter(i);
                        fn->AddCallParameter(fn->Parameter(0));
                    fn->AddWhileCondition(fn->EndCall());
                    fn->BeginCall(INCREMENT_OPERATOR);
                        fn->AddCallParameter(i);
                    fn->EndCall();
                fn->EndWhile();
                fn->CreateReturn(i32->CreateConstant(0));
            }
            fn->End();
        }

        {
            auto fn = ctx->DeclareFunction("Main", { { "argc", i32 }, { "argv", charTy->PointerTo()->PointerTo() } }, i32, false, true);
            fn->Begin();
            {
                fn->BeginCall("IfTest");
                    fn->BeginCall(v2Ty);
                        fn->AddCallParameter(f32->CreateConstant(0));
                        fn->AddCallParameter(f32->CreateConstant(0));
                    fn->AddCallParameter(fn->EndCall());
                    fn->BeginCall(v2Ty);
                        fn->AddCallParameter(f32->CreateConstant(0));
                        fn->AddCallParameter(f32->CreateConstant(0));
                    fn->AddCallParameter(fn->EndCall());
                fn->EndCall();

                auto ptr = fn->Malloc(i32);
                fn->BeginCall(STAR_OPERATOR);
                    fn->AddCallParameter(ptr);
                i32->Construct(fn, fn->EndCall(), {i32->CreateConstant(32)});
                fn->Free(ptr);

                fn->BeginCall("printf");
                    fn->BeginCall(ctx->FindFunction(PLUS_OPERATOR));
                        fn->AddCallParameter(ctx->CreateStringConstant("%p: %i\n"));
                    fn->AddCallParameter(fn->EndCall());
                    fn->AddCallParameter(ptr);
                    fn->BeginCall(STAR_OPERATOR);
                        fn->AddCallParameter(ptr);
                    fn->AddCallParameter(fn->EndCall());
                fn->EndCall();

                fn->CreateReturn(i32->CreateConstant(0));
            }
            fn->End();
        }

        ctx->Module()->dump();
        jit.AddModule(move(mod));
        puts("\n\n\n----------------------------------------------------------------------------------------------------------------------------------------");
        ctx->Module()->dump();

        auto symbol = jit.FindSymbol("Main");
        if (auto e = symbol.takeError())
        {
            Kaey::print("Can't find Main function!\n");
            return -1;
        }

        Kaey::print("Compilation Success!\nExecuting target main.\n\n\n");

        return ((int(*)(int, char**))symbol->getAddress())(argc, argv);
    }
    catch (std::exception& e)
    {
        std::puts(e.what());
        return -1;
    }
}

void DeclareIntFns(ModuleContext* ctx)
{
    auto i32 = ctx->GetIntegerType(32, true);
    auto r32 = i32->ReferenceTo();

    auto declareUnary = [=](string name, Type* valType, Type* returnType, string_view op)
    {
        auto fn = ctx->DeclareFunction(move(name), { { "v", valType } }, returnType);
        fn->Begin();
            fn->BeginCall(ctx->FindFunction(op));
                fn->AddCallParameter(fn->Parameter(0));
            fn->CreateReturn(fn->EndCall());
        fn->End();
    };

    auto declareBinary = [=](string name, Type* lhsType, Type* rhsType, Type* returnType, string_view op)
    {
        auto fn = ctx->DeclareFunction(move(name), { { "lhs", lhsType }, { "rhs", rhsType } }, returnType);
        fn->Begin();
            fn->BeginCall(ctx->FindFunction(op));
                fn->AddCallParameter(fn->Parameter(0));
                fn->AddCallParameter(fn->Parameter(1));
            fn->CreateReturn(fn->EndCall());
        fn->End();
    };

    declareUnary("inc", r32, r32, INCREMENT_OPERATOR);
    declareUnary("dec", r32, r32, DECREMENT_OPERATOR);
    declareUnary("postInc", r32, i32, POST_INCREMENT_OPERATOR);
    declareUnary("postDec", r32, i32, POST_DECREMENT_OPERATOR);

    declareUnary("unaryPlus",  r32, r32, PLUS_OPERATOR);
    declareUnary("unaryMinus", i32, i32, MINUS_OPERATOR);

    declareBinary("plus",  i32, i32, i32, PLUS_OPERATOR);
    declareBinary("minus", i32, i32, i32, MINUS_OPERATOR);
    declareBinary("star",  i32, i32, i32, STAR_OPERATOR);
    declareBinary("slash", i32, i32, i32, SLASH_OPERATOR);
    declareBinary("mod",   i32, i32, i32, MODULO_OPERATOR);

    declareBinary("plusAssign",  r32, i32, r32, PLUS_ASSIGN_OPERATOR);
    declareBinary("minusAssign", r32, i32, r32, MINUS_ASSIGN_OPERATOR);
    declareBinary("starAssign",  r32, i32, r32, STAR_ASSIGN_OPERATOR);
    declareBinary("slashAssign", r32, i32, r32, SLASH_ASSIGN_OPERATOR);
    declareBinary("modAssign",   r32, i32, r32, MODULO_ASSIGN_OPERATOR);

    declareUnary("not", i32, i32, TILDE_OPERATOR);
    declareBinary("or",  i32, i32, i32, PIPE_OPERATOR);
    declareBinary("and", i32, i32, i32, AMPERSAND_OPERATOR);
    declareBinary("xor", i32, i32, i32, CARET_OPERATOR);
    declareBinary("leftShift", i32, i32, i32, LEFT_SHIFT_OPERATOR);
    declareBinary("rightShift", i32, i32, i32, RIGHT_SHIFT_OPERATOR);

    declareBinary("orAssign",  r32, i32, r32, PIPE_ASSIGN_OPERATOR);
    declareBinary("andAssign", r32, i32, r32, AMPERSAND_ASSIGN_OPERATOR);
    declareBinary("xorAssign", r32, i32, r32, CARET_ASSIGN_OPERATOR);
    declareBinary("leftShiftAssign", r32, i32, r32, LEFT_SHIFT_ASSIGN_OPERATOR);
    declareBinary("rightShiftAssign", r32, i32, r32, RIGHT_SHIFT_ASSIGN_OPERATOR);

}

void DeclareFloatFns(ModuleContext* ctx)
{
    auto f32 = ctx->GetFloatType();
    auto r32 = f32->ReferenceTo();

    auto declareUnary = [=](string name, Type* valType, Type* returnType, string_view op)
    {
        auto fn = ctx->DeclareFunction(move(name), { { "v", valType } }, returnType);
        fn->Begin();
            fn->BeginCall(ctx->FindFunction(op));
                fn->AddCallParameter(fn->Parameter(0));
            fn->CreateReturn(fn->EndCall());
        fn->End();
    };

    auto declareBinary = [=](string name, Type* lhsType, Type* rhsType, Type* returnType, string_view op)
    {
        auto fn = ctx->DeclareFunction(move(name), { { "lhs", lhsType }, { "rhs", rhsType } }, returnType);
        fn->Begin();
            fn->BeginCall(ctx->FindFunction(op));
                fn->AddCallParameter(fn->Parameter(0));
                fn->AddCallParameter(fn->Parameter(1));
            fn->CreateReturn(fn->EndCall());
        fn->End();
    };

    declareUnary("inc", r32, r32, INCREMENT_OPERATOR);
    declareUnary("dec", r32, r32, DECREMENT_OPERATOR);
    declareUnary("postInc", r32, f32, POST_INCREMENT_OPERATOR);
    declareUnary("postDec", r32, f32, POST_DECREMENT_OPERATOR);

    declareUnary("unaryPlus", r32, r32, PLUS_OPERATOR);
    declareUnary("unaryMinus", f32, f32, MINUS_OPERATOR);

    declareBinary("plus", f32, f32, f32, PLUS_OPERATOR);
    declareBinary("minus", f32, f32, f32, MINUS_OPERATOR);
    declareBinary("star", f32, f32, f32, STAR_OPERATOR);
    declareBinary("slash", f32, f32, f32, SLASH_OPERATOR);

    declareBinary("power", f32, f32, f32, CARET_OPERATOR);

    declareBinary("plusAssign", r32, f32, r32, PLUS_ASSIGN_OPERATOR);
    declareBinary("minusAssign", r32, f32, r32, MINUS_ASSIGN_OPERATOR);
    declareBinary("starAssign", r32, f32, r32, STAR_ASSIGN_OPERATOR);
    declareBinary("slashAssign", r32, f32, r32, SLASH_ASSIGN_OPERATOR);
    declareBinary("caretAssign", r32, f32, r32, CARET_ASSIGN_OPERATOR);


}
