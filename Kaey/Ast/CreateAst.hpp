#pragma once
#include "Module.hpp"

#include "Kaey/Parser/KaeyParser.hpp"

namespace Kaey::Ast
{
    Module* CreateAst(SystemModule* sys, Parser::Module* parseTree);
}
