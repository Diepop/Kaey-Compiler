#include "Statements.hpp"

#include "Module.hpp"

namespace Kaey::Ast
{
    void Statement::AddAttribute(vector<Expression*> exprs)
    {
        attributes.emplace_back(System()->CreateObject<AttributeDeclaration>(this, move(exprs)));
    }

}
