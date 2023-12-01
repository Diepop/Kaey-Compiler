#pragma once
#include "Kaey/Ast/IValueVisitor.hpp"

namespace Kaey::Ast
{
    class StatementStreamerVisitor;
    class ValueStreamerVisitor : IValueVisitor<std::ostream&>
    {
        friend StatementStreamerVisitor;
        ValueStreamerVisitor(std::ostream& os) : os(os) {  }
        std::ostream& Visit(VariableReference* arg) override;
        std::ostream& Visit(TernaryOperator* op) override;
        std::ostream& Visit(OperatorCall* op) override;
        std::ostream& Visit(UnaryOperatorCall* op) override;
        std::ostream& Visit(BinaryOperatorCall* op) override;
        std::ostream& Visit(IntegerConstant* i) override;
        std::ostream& Visit(FloatingConstant* fl) override;
        std::ostream& Visit(CharacterConstant* v) override;
        std::ostream& Visit(ArrayConstant* arr) override;
        std::ostream& Visit(ParenthesisOperator*) override;
        std::ostream& Visit(BracketsOperator*) override;
        std::ostream& Visit(PlusOperator*) override;
        std::ostream& Visit(MinusOperator*) override;
        std::ostream& Visit(StarOperator*) override;
        std::ostream& Visit(SlashOperator*) override;
        std::ostream& Visit(ModuloOperator*) override;
        std::ostream& Visit(IncrementOperator*) override;
        std::ostream& Visit(DecrementOperator*) override;
        std::ostream& Visit(AmpersandOperator*) override;
        std::ostream& Visit(PipeOperator*) override;
        std::ostream& Visit(CaretOperator*) override;
        std::ostream& Visit(TildeOperator*) override;
        std::ostream& Visit(LeftShiftOperator*) override;
        std::ostream& Visit(RightShiftOperator*) override;
        std::ostream& Visit(ExclamationOperator*) override;
        std::ostream& Visit(AndOperator*) override;
        std::ostream& Visit(OrOperator*) override;
        std::ostream& Visit(EqualOperator*) override;
        std::ostream& Visit(NotEqualOperator*) override;
        std::ostream& Visit(LessOperator*) override;
        std::ostream& Visit(GreaterOperator*) override;
        std::ostream& Visit(LessEqualOperator*) override;
        std::ostream& Visit(GreaterEqualOperator*) override;
        std::ostream& Visit(SpaceshipOperator*) override;
        std::ostream& Visit(AssignmentOperator*) override;
        std::ostream& Visit(PlusAssignmentOperator*) override;
        std::ostream& Visit(MinusAssignmentOperator*) override;
        std::ostream& Visit(StarAssignmentOperator*) override;
        std::ostream& Visit(SlashAssignmentOperator*) override;
        std::ostream& Visit(ModuloAssignmentOperator*) override;
        std::ostream& Visit(AmpersandAssignmentOperator*) override;
        std::ostream& Visit(PipeAssignmentOperator*) override;
        std::ostream& Visit(CaretAssignmentOperator*) override;
        std::ostream& Visit(LeftShiftAssignmentOperator*) override;
        std::ostream& Visit(RightShiftAssignmentOperator*) override;
        std::ostream& Visit(ConstructOperatorCall* arg) override;
        std::ostream& Visit(TupleExpression* arg) override;

    private:
        std::ostream& os;

        template<class Range, class Delimiter>
        std::ostream& DispatchRange(Range&& range, Delimiter&& delimiter);
    };
    
}
