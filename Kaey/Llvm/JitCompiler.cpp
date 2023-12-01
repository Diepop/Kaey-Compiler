#include "JitCompiler.hpp"

#define vars() auto& [es, objectLinkingLayer, compileLayer, dataLayout, mangle, ctx, mainLib, builder, lam, cgam, fam, mam, pass] = *this->v

namespace Kaey::Llvm
{
    struct JitCompiler::Vars
    {
        llvm::orc::ExecutionSession es;
        llvm::orc::RTDyldObjectLinkingLayer objectLinkingLayer;
        llvm::orc::IRCompileLayer compileLayer;
        llvm::DataLayout dataLayout;
        llvm::orc::MangleAndInterner mangle;
        llvm::orc::ThreadSafeContext ctx;
        llvm::orc::JITDylib* mainLib;
        //Optimizer
        llvm::PassBuilder builder;
        llvm::LoopAnalysisManager lam;
        llvm::CGSCCAnalysisManager cgam;
        llvm::FunctionAnalysisManager fam;
        llvm::ModuleAnalysisManager mam;
        std::optional<llvm::ModulePassManager> pass;

        Vars(llvm::orc::JITTargetMachineBuilder jtmb, const llvm::DataLayout& dataLayout, llvm::OptimizationLevel optimizationLevel) :
            es(cantFail(llvm::orc::SelfExecutorProcessControl::Create())),
            objectLinkingLayer(es, [] { return std::make_unique<llvm::SectionMemoryManager>(); }),
            compileLayer(es, objectLinkingLayer, std::make_unique<llvm::orc::ConcurrentIRCompiler>(std::move(jtmb))),
            dataLayout(dataLayout), mangle(es, this->dataLayout),
            ctx(std::make_unique<llvm::LLVMContext>()), mainLib(&cantFail(es.createJITDylib("main")))
        {
            if (optimizationLevel != llvm::OptimizationLevel::O0)
            {
                builder.registerModuleAnalyses(mam);
                builder.registerCGSCCAnalyses(cgam);
                builder.registerFunctionAnalyses(fam);
                builder.registerLoopAnalyses(lam);
                builder.crossRegisterProxies(lam, fam, cgam, mam);
                pass.emplace(builder.buildPerModuleDefaultPipeline(optimizationLevel));
            }
            mainLib->addGenerator(cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess('\0')));
            objectLinkingLayer.setOverrideObjectFlagsWithResponsibilityFlags(true);
            objectLinkingLayer.setAutoClaimResponsibilityForObjectSymbols(true);
        }

        Vars(llvm::orc::JITTargetMachineBuilder jtmb, llvm::OptimizationLevel optimizationLevel) : Vars(std::move(jtmb), cantFail(jtmb.getDefaultDataLayoutForTarget()), optimizationLevel)
        {

        }

        Vars(llvm::OptimizationLevel optimizationLevel) : Vars([]
        {
            llvm::InitializeNativeTarget();
            llvm::InitializeNativeTargetAsmParser();
            llvm::InitializeNativeTargetAsmPrinter();
            return cantFail(llvm::orc::JITTargetMachineBuilder::detectHost());
        }(), optimizationLevel)
        {

        }

        ~Vars()
        {
            cantFail(es.endSession());
        }

    };

    JitCompiler::JitCompiler(OptimizationLevel optimizationLevel) : v(std::make_unique<Vars>([=]
    {
        switch (optimizationLevel)
        {
        case OptimizationLevel::O0: return llvm::OptimizationLevel::O0;
        case OptimizationLevel::O1: return llvm::OptimizationLevel::O1;
        case OptimizationLevel::O2: return llvm::OptimizationLevel::O2;
        case OptimizationLevel::O3: return llvm::OptimizationLevel::O3;
        case OptimizationLevel::Os: return llvm::OptimizationLevel::Os;
        case OptimizationLevel::Oz: return llvm::OptimizationLevel::Oz;
        default: throw std::runtime_error("Invalid Enumeration!");
        }
    }()))
    {
        
    }

    JitCompiler::~JitCompiler() = default;

    llvm::DataLayout& JitCompiler::DataLayout() const
    {
        return v->dataLayout;
    }

    llvm::LLVMContext& JitCompiler::Context() const
    {
        return *v->ctx.getContext();
    }

    void JitCompiler::AddModule(std::unique_ptr<llvm::Module> mod)
    {
        vars();
        if (pass) pass->run(*mod, mam);
        mod->setDataLayout(dataLayout);
        cantFail(compileLayer.add(*mainLib, llvm::orc::ThreadSafeModule(move(mod), ctx)));
    }

    llvm::Expected<llvm::JITEvaluatedSymbol> JitCompiler::FindSymbol(llvm::StringRef Name)
    {
        vars();
        llvm::orc::JITDylib* libs[]{mainLib};
        return es.lookup(libs, mangle(Name));
    }

    void JitCompiler::AddSymbol(llvm::StringRef name, void* ptr)
    {
        vars();
        llvm::orc::SymbolMap m;
        m[mangle(name)] = {llvm::pointerToJITTargetAddress(ptr), llvm::JITSymbolFlags()};
        cantFail(mainLib->define(absoluteSymbols(m)));
    }
}
