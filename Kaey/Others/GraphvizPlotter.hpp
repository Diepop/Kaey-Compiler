#pragma once
#include "Kaey/Parser/KaeyParser.hpp"

namespace Graphviz
{
    using namespace Kaey::Lexer;
    using namespace Kaey::Parser;
    using namespace Kaey;

    struct GraphvizPlotter : Expression::Visitor<string_view>, Statement::Visitor<string_view>, BaseType::Visitor<string_view>
    {
        using Pair = std::pair<string, string>;
        using Map = std::map<string, vector<Pair>>;

        explicit GraphvizPlotter(bool ignoreParenthesis = true);

        void Plot(std::ostream& os, Statement* root);

        void Plot(std::ostream& os, const Module* mod);

        string_view Visit(Statement* arg);

        string_view Visit(Expression* arg);

        string_view Visit(BaseType* arg);

        string_view Visit(BooleanExpression* arg);

        string_view Visit(IntegerExpression* arg);

        string_view Visit(FloatingExpression* arg);

        string_view Visit(UnaryOperatorExpression* expr);

        string_view Visit(BinaryOperatorExpression* expr);

        string_view Visit(CallExpression* expr);

        string_view Visit(IdentifierExpression* expr);

        string_view Visit(StringLiteralExpression* arg);

        string_view Visit(TupleExpression* arg);

        string_view Visit(SubscriptOperator* arg);

        string_view Visit(TypeCastExpression* arg);

        string_view Visit(IdentifierType* arg);

        string_view Visit(DecoratedType* arg);

        string_view Visit(ArrayType* arg);

        string_view Visit(TupleType* arg);

        string_view Visit(VariantType* arg);

        string_view Visit(ExpressionStatement* arg);

        string_view Visit(BlockStatement* arg);

        string_view Visit(IfStatement* arg);

        string_view Visit(WhileStatement* arg);

        string_view Visit(ForStatement* arg);

        string_view Visit(SwitchStatement* arg);

        string_view Visit(ParenthisedExpression* arg);

        string_view Visit(VariableDeclaration* arg);

        string_view Visit(PropertyDeclaration* arg);

        string_view Visit(ClassDeclaration* arg);

        string_view Visit(FunctionDeclaration* arg);

        string_view Visit(AttributeDeclaration* arg);

        string_view Visit(ConstructorDeclaration* arg);

        string_view Visit(TernaryExpression* arg);

        string_view Visit(OperatorDeclaration* arg);

        string_view Visit(MemberAccessExpression* arg);

        string_view Visit(NewExpression* arg);

        string_view Visit(NamedOperatorExpression* arg);

        string_view Visit(NamedTypeOperatorExpression* arg);

        template<class Res = void, class... Args>
        auto Dispatch(auto ptr, Args&&... args)
        {
            if constexpr (sizeof...(Args) > 0)
                return Kaey::Dispatch<Res>(ptr, std::forward<Args>(args)...);
            else return Kaey::Dispatch<Res>(ptr, this);
        }

    private:
        Map nodes;
        vector<string> relationships;
        bool ignoreParenthesis;
        vector<string> indexes;

        ArrayView<string> NamedIndexes(size_t size);

        string_view AddNode(vector<Pair> attrs);

        void AddRelationship(string_view n1, string_view n2, vector<Pair> attrs = {});

        template<class Ptr>
        string_view UnimplementedNode(Ptr* arg) { return AddNode({ { "label", "'{}' is Unimplemented!"_f(arg->NameOf()) }, { "color", "red" } }); }

    };

}
