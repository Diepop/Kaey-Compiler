#include "Kaey/Ast/CreateAst.hpp"

#include "Kaey/Llvm/JitCompiler.hpp"
#include "Kaey/Llvm/ModuleContext.hpp"

#include "Kaey/Compiler.hpp"

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

int main(int argc, char* argv[])
{
    try
    {
        if (argc <= 1)
        {
            puts("error: no input files");
            return -1;
        }
        std::filesystem::path path{ argv[1] };
        if (!exists(path) || !is_regular_file(path))
        {
            puts("error: file not found");
            return -1;
        }
        auto ss = Kaey::Lexer::StringStream::FromFile(path);
        auto ts = Kaey::Lexer::TokenStream(ss);
        auto fileName = path.stem().string();
        auto parseTree = Kaey::Parser::Module::Parse(fileName, ts);
        puts("Parsing completed!");

        Kaey::Ast::SystemModule sys;
        auto astMod = CreateAst(&sys, &parseTree);
        puts("\n");

        Kaey::Llvm::JitCompiler jit;
        auto mod = std::make_unique<llvm::Module>(fileName, jit.Context());

        PrintModule(mod.get());

        auto cMod = std::make_unique<Kaey::Llvm::ModuleContext>(jit.Context(), mod.get(), &jit.DataLayout());
        CompileModule(cMod.get(), astMod);

        PrintModule();
        jit.AddModule(move(mod));
        PrintModule();

        auto symbol = jit.FindSymbol("Main");
        if (auto e = symbol.takeError())
        {
            puts("Can't find Main function!");
            return -1;
        }

        puts("Compilation Success!\nExecuting target main.\n\n");
        return symbol->getAddress().toPtr<int(*)(int, char**)>()(argc, argv);
    }
    catch (std::ios::failure& e)
    {
        Kaey::print("error: couldn't open file! {}\n", e.what());
    }
    catch (std::runtime_error& e)
    {
        puts(e.what());
    }
    return -1;
}
