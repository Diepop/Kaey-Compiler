#pragma once

namespace Kaey::Llvm
{
    enum class OptimizationLevel
    {
        O0,
        O1,
        O2,
        O3,
        Os,
        Oz,
    };

    struct JitCompiler
    {
        JitCompiler(OptimizationLevel optimizationLevel = OptimizationLevel::O3);

        JitCompiler(const JitCompiler&) = delete;
        JitCompiler(JitCompiler&&) = delete;

        JitCompiler& operator=(const JitCompiler&) = delete;
        JitCompiler& operator=(JitCompiler&&) = delete;

        ~JitCompiler();

        llvm::DataLayout& DataLayout() const;
        llvm::LLVMContext& Context() const;
        void AddModule(std::unique_ptr<llvm::Module> mod);
        llvm::Expected<llvm::orc::ExecutorSymbolDef> FindSymbol(llvm::StringRef name);
        void AddSymbol(llvm::StringRef name, void* ptr);
    private:
        struct Vars;
        std::unique_ptr<Vars> v;
    };

}
