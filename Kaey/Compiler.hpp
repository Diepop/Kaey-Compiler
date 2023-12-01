#pragma once

namespace Kaey
{
    namespace Ast
    {
        struct Module;
    }

    namespace Llvm
    {
        struct ModuleContext;
        void CompileModule(ModuleContext* mod, Ast::Module* ast);
    }
}
