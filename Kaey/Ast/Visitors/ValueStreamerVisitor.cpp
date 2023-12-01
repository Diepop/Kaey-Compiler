#include "ValueStreamerVisitor.hpp"

#include "Expression/VariableReference.hpp"
#include "Kaey/Ast/Function.hpp"
#include "Kaey/Ast/Operators.hpp"
#include "Kaey/Asts.hpp"

namespace Kaey::Ast
{
    template <class Range, class Delimiter>
    std::ostream& ValueStreamerVisitor::DispatchRange(Range&& range, Delimiter&& delimiter)
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

    string_view Format(char d)
    {
        static char arr[]{ '\0', '\0' };
        switch (d)
        {
        case '\n': return "\\n";
        default: return &(arr[0] = d);
        }
    }

    std::ostream& ValueStreamerVisitor::Visit(VariableReference* arg)
    {
        return os << arg->Name();
    }

    std::ostream& ValueStreamerVisitor::Visit(TernaryOperator* op)
    {
        return Dispatch(op->Condition()) << " ? ", Dispatch(op->TrueExpression()) << " : ", Dispatch(op->FalseExpression());
    }

    std::ostream& ValueStreamerVisitor::Visit(OperatorCall* op)
    {
        return os << op->Callee()->Name() << '(', DispatchRange(op->Arguments(), ", ") << ')';
    }

    std::ostream& ValueStreamerVisitor::Visit(UnaryOperatorCall* op)
    {
        auto ptr = Dispatch<std::ostream*>(op->Callee(),
            [&](PostIncrementOperator*) -> std::ostream* { return &(Dispatch(op->Operand()), os << "++"); },
            [&](PostDecrementOperator*) -> std::ostream* { return &(Dispatch(op->Operand()), os << "--"); }
        );
        if (ptr) return *ptr;
        return Dispatch(op->Callee()), Dispatch(op->Operand());
    }

    std::ostream& ValueStreamerVisitor::Visit(BinaryOperatorCall* op)
    {
        
        return Dispatch(op->LeftOperand()) << ' ', Dispatch(op->Callee()) << ' ', Dispatch(op->RightOperand());
    }

    std::ostream& ValueStreamerVisitor::Visit(IntegerConstant* i)
    {
        return os << i->Value();
    }

    std::ostream& ValueStreamerVisitor::Visit(FloatingConstant* fl)
    {
        return os << fl->Value();
    }

    std::ostream& ValueStreamerVisitor::Visit(CharacterConstant* v)
    {
        return os << v->Value();
    }

    std::ostream& ValueStreamerVisitor::Visit(ArrayConstant* arr)
    {
        if (arr->TypeOfArray()->UnderlyingType()->Is<CharacterType>())
        {
            os << '"';
            for (auto v : arr->Value() | Cast<CharacterConstant>())
                os << Format(v->Value());
            return os << '"';
        }
        return os << '[', DispatchRange(arr->Value(), ", ") << ']';
    }

    std::ostream& ValueStreamerVisitor::Visit(ParenthesisOperator*)
    {
        return os << "()";
    }

    std::ostream& ValueStreamerVisitor::Visit(BracketsOperator*)
    {
        return os << "[]";
    }

    std::ostream& ValueStreamerVisitor::Visit(PlusOperator*)
    {
        return os << '+';
    }

    std::ostream& ValueStreamerVisitor::Visit(MinusOperator*)
    {
        return os << '-';
    }

    std::ostream& ValueStreamerVisitor::Visit(StarOperator*)
    {
        return os << '*';
    }

    std::ostream& ValueStreamerVisitor::Visit(SlashOperator*)
    {
        return os << '/';
    }

    std::ostream& ValueStreamerVisitor::Visit(ModuloOperator*)
    {
        return os << '%';
    }

    std::ostream& ValueStreamerVisitor::Visit(IncrementOperator*)
    {
        return os << "++";
    }

    std::ostream& ValueStreamerVisitor::Visit(DecrementOperator*)
    {
        return os << "--";
    }

    std::ostream& ValueStreamerVisitor::Visit(AmpersandOperator*)
    {
        return os << '&';
    }

    std::ostream& ValueStreamerVisitor::Visit(PipeOperator*)
    {
        return os << '|';
    }

    std::ostream& ValueStreamerVisitor::Visit(CaretOperator*)
    {
        return os << '^';
    }

    std::ostream& ValueStreamerVisitor::Visit(TildeOperator*)
    {
        return os << '~';
    }

    std::ostream& ValueStreamerVisitor::Visit(LeftShiftOperator*)
    {
        return os << "<<";
    }

    std::ostream& ValueStreamerVisitor::Visit(RightShiftOperator*)
    {
        return os << ">>";
    }

    std::ostream& ValueStreamerVisitor::Visit(ExclamationOperator*)
    {
        return os << '!';
    }

    std::ostream& ValueStreamerVisitor::Visit(AndOperator*)
    {
        return os << "&&";
    }

    std::ostream& ValueStreamerVisitor::Visit(OrOperator*)
    {
        return os << "||";
    }

    std::ostream& ValueStreamerVisitor::Visit(EqualOperator*)
    {
        return os << "==";
    }

    std::ostream& ValueStreamerVisitor::Visit(NotEqualOperator*)
    {
        return os << "!=";
    }

    std::ostream& ValueStreamerVisitor::Visit(LessOperator*)
    {
        return os << '<';
    }

    std::ostream& ValueStreamerVisitor::Visit(GreaterOperator*)
    {
        return os << '>';
    }

    std::ostream& ValueStreamerVisitor::Visit(LessEqualOperator*)
    {
        return os << "<=";
    }

    std::ostream& ValueStreamerVisitor::Visit(GreaterEqualOperator*)
    {
        return os << ">=";
    }

    std::ostream& ValueStreamerVisitor::Visit(SpaceshipOperator*)
    {
        return os << "<=>";
    }

    std::ostream& ValueStreamerVisitor::Visit(AssignmentOperator*)
    {
        return os << '=';
    }

    std::ostream& ValueStreamerVisitor::Visit(PlusAssignmentOperator*)
    {
        return os << "+=";
    }

    std::ostream& ValueStreamerVisitor::Visit(MinusAssignmentOperator*)
    {
        return os << "-=";
    }

    std::ostream& ValueStreamerVisitor::Visit(StarAssignmentOperator*)
    {
        return os << "*=";
    }

    std::ostream& ValueStreamerVisitor::Visit(SlashAssignmentOperator*)
    {
        return os << "/=";
    }

    std::ostream& ValueStreamerVisitor::Visit(ModuloAssignmentOperator*)
    {
        return os << "%=";
    }

    std::ostream& ValueStreamerVisitor::Visit(AmpersandAssignmentOperator*)
    {
        return os << "&=";
    }

    std::ostream& ValueStreamerVisitor::Visit(PipeAssignmentOperator*)
    {
        return os << "|=";
    }

    std::ostream& ValueStreamerVisitor::Visit(CaretAssignmentOperator*)
    {
        return os << "^=";
    }

    std::ostream& ValueStreamerVisitor::Visit(LeftShiftAssignmentOperator*)
    {
        return os << "<<=";
    }

    std::ostream& ValueStreamerVisitor::Visit(RightShiftAssignmentOperator*)
    {
        return os << ">>=";
    }

    std::ostream& ValueStreamerVisitor::Visit(ConstructOperatorCall* arg)
    {
        return os << arg->Type()->Name() << '(', DispatchRange(arg->Arguments(), ", ") << ')';
    }

    std::ostream& ValueStreamerVisitor::Visit(TupleExpression* arg)
    {
        return os << '(', DispatchRange(arg->Arguments(), ", ") << ')';
    }
}
