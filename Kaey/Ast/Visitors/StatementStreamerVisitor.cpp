// ReSharper disable CppClangTidyClangDiagnosticComma
#include "StatementStreamerVisitor.hpp"
#include "ModuleContext.hpp"
#include "Expression/Function.hpp"
#include "Statement/BlockStatement.hpp"
#include "Statement/FunctionDeclaration.hpp"
#include "Statement/IfStatement.hpp"
#include "Statement/ReturnStatement.hpp"
#include "Statement/VariableDeclaration.hpp"

namespace Kaey::Ast
{
    template <class Range, class Delimiter>
    std::ostream& StatementStreamerVisitor::DispatchRange(Range&& range, Delimiter&& delimiter)
    {
        auto it = std::begin(range);
        auto end = std::end(range);
        if (it == end) return os;
        Dispatch(*it++);
        while (it != end)
        {
            os << delimiter;
            Dispatch(*it++);
        }
        return os;
    }

    template <class Fn>
    std::ostream& StatementStreamerVisitor::PushSpace(Fn&& fn)
    {
        spaces += "  ";
        fn();
        spaces.resize(spaces.size() - 2);
        return os;
    }

    StatementStreamerVisitor::StatementStreamerVisitor(std::ostream& os): os(os), v(os), spaces("\n")
    {
        
    }

    std::ostream& StatementStreamerVisitor::Visit(BlockStatement* block)
    {
        string_view sv = spaces;
        sv.remove_suffix(1);
        if (!sv.empty()) sv.remove_suffix(2);
        os << sv << "{";
        PushSpace([&]
        {
            for (auto statement : block->Statements())
                os << spaces, Dispatch(statement);
        });
        return os << spaces << "}";
    }

    std::ostream& StatementStreamerVisitor::Visit(ExpressionStatement* statement)
    {
        return v.Dispatch(statement->Expression()) << ';';
    }

    std::ostream& StatementStreamerVisitor::Visit(ReturnStatement* statement)
    {
        return os << "return ", v.Dispatch(statement->Expression()) << ';';
    }

    std::ostream& StatementStreamerVisitor::Visit(FunctionDeclaration* fn)
    {
        if (!!(fn->FunctionModifiers() & FunctionModifier::External)) os << "extern ";
        if (!!(fn->FunctionModifiers() & FunctionModifier::DisableMangling)) os << R"("C" )";
        os << "[] " << fn->MasterFunction()->Name() << '(';
        for (auto parameter : fn->Parameters())
        {
            if (parameter != fn->Parameters().front()) os << ", ";
            Format(parameter);
        }
        os << ") : " << fn->ReturnType()->Name();
        auto body = fn->Body();
        return body ? os << '\n', Visit(body) : os << ";";
    }

    std::ostream& StatementStreamerVisitor::Visit(VariableDeclaration* dcl)
    {
        return os << "var ", Format(dcl) << ";";
    }

    std::ostream& StatementStreamerVisitor::Visit(IfStatement* arg)
    {
        os << "if (", v.Dispatch(arg->Condition()) << ")\n";
        PushSpace([&] { DispatchRange(arg->Statements(), '\n'); });
        return os;
    }

    std::ostream& StatementStreamerVisitor::Visit(ElseStatement* arg)
    {
        return os;
    }

    std::ostream& StatementStreamerVisitor::Visit(WhileStatement* arg)
    {
        return os;
    }

    std::ostream& StatementStreamerVisitor::Visit(ForStatement* arg)
    {
        return os;
    }

    std::ostream& StatementStreamerVisitor::Visit(ClassDeclarationStatement* arg)
    {
        return os;
    }

    std::ostream& StatementStreamerVisitor::operator()(ModuleContext* mod)
    {
        return DispatchRange(mod->Statements(), spaces);
    }

    std::ostream& StatementStreamerVisitor::Format(VariableDeclaration* decl)
    {
        auto init = decl->Initializer();
        return os << decl->Name() << ": " << decl->Type()->Name(), init ? os << " = ", v.Dispatch(init) : os;
    }

}
