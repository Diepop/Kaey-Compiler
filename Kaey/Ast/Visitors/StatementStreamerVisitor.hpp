#pragma once
#include "Kaey/Ast/IStatementVisitor.hpp"
#include "ValueStreamerVisitor.hpp"

namespace Kaey::Ast
{
    struct ModuleContext;

    class StatementStreamerVisitor : IStatementVisitor<std::ostream&>
    {
        std::ostream& os;
        ValueStreamerVisitor v;
        std::string spaces;

        std::ostream& Format(VariableDeclaration* decl);

        template<class Range, class Delimiter>
        std::ostream& DispatchRange(Range&& range, Delimiter&& delimiter);

        template<class Fn>
        std::ostream& PushSpace(Fn&& fn);

        StatementStreamerVisitor(std::ostream& os);

        std::ostream& Visit(BlockStatement* block) override;

        std::ostream& Visit(ExpressionStatement* statement) override;

        std::ostream& Visit(ReturnStatement* statement) override;

        std::ostream& Visit(FunctionDeclaration* fn) override;

        std::ostream& Visit(VariableDeclaration* dcl) override;

        std::ostream& Visit(IfStatement* arg) override;

        std::ostream& Visit(ElseStatement* arg) override;

        std::ostream& Visit(WhileStatement* arg) override;

        std::ostream& Visit(ForStatement* arg) override;

        std::ostream& Visit(ClassDeclarationStatement* arg) override;

        std::ostream& operator()(ModuleContext* mod);

    private:
        
    };
    
}
