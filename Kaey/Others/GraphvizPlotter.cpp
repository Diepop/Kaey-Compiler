#include "GraphvizPlotter.hpp"

namespace Graphviz
{
    GraphvizPlotter::GraphvizPlotter(bool ignoreParenthesis) : ignoreParenthesis(ignoreParenthesis)
    {
        
    }

    void GraphvizPlotter::Plot(std::ostream& os, Statement* root)
    {
        Dispatch(root);
        os << "digraph g{\n";
        os << R"(label = "Comic Sans MS")" << '\n';
        os << R"(fontname = "Comic Sans MS")" << '\n';
        for (auto& [name, attrs] : nodes)
        {
            os << "  " << name;
            for (auto& [prop, value] : attrs)
                os << "[" << prop << "=\"" << value << "\"]";
            os << "\n";
        }
        os << "\n";
        for (auto& relation : relationships)
            os << "  " << relation << "\n";
        os << "}";
        nodes.clear();
        relationships.clear();
    }

    void GraphvizPlotter::Plot(std::ostream& os, const Module* mod)
    {
        auto root = AddNode({ { "label", mod->Name } });
        for (auto s : mod->Statements)
            AddRelationship(root, Dispatch(s));
        os << "digraph g{\n";
        for (auto& [name, attrs] : nodes)
        {
            os << "  " << name;
            for (auto& [prop, value] : attrs)
                os << "[" << prop << "=\"" << value << "\"]";
            os << "\n";
        }
        os << "\n";
        for (auto& relation : relationships)
            os << "  " << relation << "\n";
        os << "}";
        nodes.clear();
        relationships.clear();
    }

    ArrayView<string> GraphvizPlotter::NamedIndexes(size_t size)
    {
        indexes.reserve(size);
        for (auto i = indexes.size(); i < size; ++i)
            indexes.emplace_back("${}"_f(i));
        return { indexes.data(), size };
    }

    string_view GraphvizPlotter::AddNode(vector<Pair> attrs)
    {
        return nodes.emplace("n{:03}"_f(nodes.size() + 1), move(attrs)).first->first;
    }

    void GraphvizPlotter::AddRelationship(string_view n1, string_view n2, vector<Pair> attrs)
    {
        if (rn::none_of(attrs, [](auto& tup) { return tup.first == "color"; }))
            attrs.emplace_back("color", "purple");
        relationships.emplace_back("{} -> {}{}"_f(n1, n2, join(attrs | vs::transform([](auto&& pair) { return "[{}=\"{}\"]"_f(pair.first, pair.second); }), "")));
    }

    string_view GraphvizPlotter::Visit(Statement* arg)
    {
        return UnimplementedNode(arg);
    }

    string_view GraphvizPlotter::Visit(Expression* arg)
    {
        return UnimplementedNode(arg);
    }

    string_view GraphvizPlotter::Visit(BaseType* arg)
    {
        return UnimplementedNode(arg);
    }

    string_view GraphvizPlotter::Visit(BooleanExpression* arg)
    {
        return AddNode({ { "label", "{}"_f(arg->Value) } });
    }

    string_view GraphvizPlotter::Visit(IntegerExpression* arg)
    {
        return AddNode({ { "label", arg->ToString() } });
    }

    string_view GraphvizPlotter::Visit(FloatingExpression* arg)
    {
        return AddNode({ { "label", arg->ToString() } });
    }

    string_view GraphvizPlotter::Visit(UnaryOperatorExpression* expr)
    {
        auto str = "operator {}($0)"_f(expr->Operation->Text());
        if (expr->IsSuffix)
            str += " suffix";
        auto name = AddNode({ { "label", move(str) } });
        AddRelationship(name, Dispatch(expr->Operand));
        return name;
    }

    string_view GraphvizPlotter::Visit(BinaryOperatorExpression* expr)
    {
        auto name = AddNode({ { "label", "$0 {} $1"_f(expr->Operation->Text()) }, { "color", "blue" } });
        AddRelationship(name, Dispatch(expr->Left));
        AddRelationship(name, Dispatch(expr->Right));
        return name;
    }

    string_view GraphvizPlotter::Visit(CallExpression* expr)
    {
        string_view name;
        bool supressArgumentsNode = false;
        if (auto id = expr->Callee->As<IdentifierExpression>())
        {
            name = AddNode({ { "label", "{}({})"_f(id->Identifier, join(NamedIndexes(expr->Arguments.size()), ", ")) }, { "color", "red" } });
            supressArgumentsNode = true;
        }
        else
        {
            name = AddNode({ { "label", "CallExpression" }, { "color", "red" } });
            relationships.emplace_back("{} -> {}"_f(name, Dispatch(expr->Callee)));
        }
        if (!expr->Arguments.empty())
        {
            string_view rels = !supressArgumentsNode ? AddNode({ { "label", "{}"_f(name) } }) : name;
            if (!supressArgumentsNode)
                AddRelationship(name, rels);
            auto i = 0;
            for (auto operand : expr->Arguments)
                AddRelationship(rels, Dispatch(operand), { { "headlabel", "${}"_f(i++) } });
        }
        return name;
    }

    string_view GraphvizPlotter::Visit(IdentifierExpression* expr)
    {
        return AddNode({ { "label", "Identifier '{}'"_f(expr->Identifier) } });
    }

    string_view GraphvizPlotter::Visit(StringLiteralExpression* arg)
    {
        string txt;
        txt.reserve(arg->Token->Text().size());
        for (auto d : arg->Token->Text())
        {
            switch (d)
            {
            case '\\':
            case '"':
                txt += '\\';
                txt += d;
            break;
            default:
                txt += d;
            break;
            }
        }
        return AddNode({ { "label", "StringLiteral: {}"_f(txt) } });
    }

    string_view GraphvizPlotter::Visit(TupleExpression* arg)
    {
        auto name = AddNode({ { "label", "Tuple" } });
        for (auto v : arg->Expressions)
        {
            auto n = AddNode({ { "label", v->ToString() } });
            AddRelationship(name, n);
        }
        return name;
    }

    string_view GraphvizPlotter::Visit(SubscriptOperator* arg)
    {
        string_view name;
        if (auto id = arg->Callee->As<IdentifierExpression>())
            name = AddNode({ { "label", "{}[{}]"_f(id->Identifier, join(NamedIndexes(arg->Arguments.size()), ", ")) }, { "color", "red" } });
        else
        {
            name = AddNode({ { "label", "this[{}]"_f(join(NamedIndexes(arg->Arguments.size()), ", ")) }, { "color", "red" } });
            AddRelationship(name, Dispatch(arg->Callee), { { "headlabel", "this" } });
        }
        if (!arg->Arguments.empty())
        {
            auto i = 0;
            for (auto operand : arg->Arguments)
                AddRelationship(name, Dispatch(operand), { { "headlabel", "${}"_f(i++) } });
        }
        return name;
    }

    string_view GraphvizPlotter::Visit(TypeCastExpression* arg)
    {
        auto name = AddNode({ { "label", "$0 {} $1"_f(arg->ConvertionType->Text()) }, { "color", "blue" } });
        AddRelationship(name, Dispatch(arg->ConvertedExpression), { { "headlabel", "$0" } });
        AddRelationship(name, Dispatch(arg->Type), { { "headlabel", "$1" } });
        return name;
    }

    string_view GraphvizPlotter::Visit(IdentifierType* arg)
    {
        return AddNode({ { "label", "{}"_f(arg->Identifier) }, { "color", "blue" } });
    }

    string_view GraphvizPlotter::Visit(DecoratedType* arg)
    {
        auto name = AddNode({ { "label", "{}"_f(arg->ToString()) }, { "color", "blue" } });
        //AddRelationship(name, Dispatch(arg->UnderlyingType));
        return name;
    }

    string_view GraphvizPlotter::Visit(ArrayType* arg)
    {
        auto len = arg->LengthExpressions.size();
        auto idx = NamedIndexes(len) | to_vector;
        idx.reserve(idx.size() + rn::count_if(arg->LengthExpressions, std::logical_not()));
        for (size_t i = 0; i < len; ++i) if (!arg->LengthExpressions[i])
            idx.insert(idx.begin() + (int)i, "");
        idx.resize(len);
        auto name = AddNode({ { "label", "Type[{}]"_f(join(idx, ", ")) }, { "color", "blue" } });
        AddRelationship(name, Dispatch(arg->UnderlyingType), { { "headlabel", "Type" } });
        auto i = 0;
        for (auto v : arg->LengthExpressions) if (v)
            AddRelationship(name, Dispatch(v), { { "headlabel", "${}"_f(i++) } });
        return name;
    }

    string_view GraphvizPlotter::Visit(TupleType* arg)
    {
        auto name = AddNode({ { "label", "Tuple Type" }, { "color", "blue" } });
        for (auto type : arg->Types)
            AddRelationship(name, Dispatch(type));
        return name;
    }

    string_view GraphvizPlotter::Visit(VariantType* arg)
    {
        auto name = AddNode({ { "label", "Variant Type" }, { "color", "blue" } });
        for (auto type : arg->UnderlyingTypes)
            AddRelationship(name, Dispatch(type));
        return name;
    }

    string_view GraphvizPlotter::Visit(ExpressionStatement* arg)
    {
        auto e = arg->UnderlyingExpression;
        return e ? Dispatch(e) : AddNode({ { "label", ";" }, { "color", "yellow" } });
    }

    string_view GraphvizPlotter::Visit(BlockStatement* arg) {
        auto name = AddNode({ { "label", "{...}" }, { "color", "yellow" } });
        for (auto statement : arg->Statements)
            AddRelationship(name, Dispatch(statement));
        return name;
    }

    string_view GraphvizPlotter::Visit(IfStatement* arg)
    {
        auto name = AddNode({ { "label", "if" }, { "color", "yellow" } });
        AddRelationship(name, Dispatch(arg->Condition), { { "headlabel", "Condition" } });
        AddRelationship(name, Dispatch(arg->TrueStatement), { { "headlabel", "True Branch" } });
        if (!arg->FalseStatement) return name;
        AddRelationship(name, Dispatch(arg->FalseStatement), { { "headlabel", "False Branch" } });
        return name;
    }

    string_view GraphvizPlotter::Visit(WhileStatement* arg)
    {
        auto name = AddNode({ { "label", "while" }, { "color", "yellow" } });
        AddRelationship(name, Dispatch(arg->Condition), { { "headlabel", "Condition" } });
        AddRelationship(name, Dispatch(arg->LoopStatement), { { "headlabel", "Loop Statement" } });
        return name;
    }

    string_view GraphvizPlotter::Visit(ForStatement* arg)
    {
        auto name = AddNode({ { "label", "for" }, { "color", "yellow" } });
        AddRelationship(name, Dispatch(arg->StartStatement), { { "headlabel", "Start Statement" } });
        AddRelationship(name, Dispatch(arg->Condition), { { "headlabel", "Condition" } });
        AddRelationship(name, Dispatch(arg->Increment), { { "headlabel", "Increment Expression" } });
        AddRelationship(name, Dispatch(arg->LoopStatement), { { "headlabel", "Loop Statement" } });
        return name;
    }

    string_view GraphvizPlotter::Visit(SwitchStatement* arg)
    {
        auto name = AddNode({ { "label", "switch" }, { "color", "yellow" } });
        AddRelationship(name, Dispatch(arg->Condition), { { "headlabel", "Condition" } });
        for (auto& [expr, stat] : arg->Cases)
        {
            auto e = expr ? Dispatch(expr) : AddNode({ { "label", "_" }, { "color", "blue" } });
            AddRelationship(name, e);
            AddRelationship(e, Dispatch(stat));
        }
        return name;
    }

    string_view GraphvizPlotter::Visit(ParenthisedExpression* arg)
    {
        if (ignoreParenthesis)
            return Dispatch(arg->UnderlyingExpression);
        auto name = AddNode({ { "label", "Parenthised" } });
        AddRelationship(name, Dispatch(arg->UnderlyingExpression), { { "headlabel", "Underlying Expression" } });
        return name;
    }

    string_view GraphvizPlotter::Visit(VariableDeclaration* arg)
    {
        auto name = AddNode({ { "label", "Variable Declaration" }, { "color", "yellow" } });
        if (arg->DeclarationToken)
            AddRelationship(name, AddNode({ { "label", (string)arg->DeclarationToken->Text() }, { "color", "purple" } }), { { "headlabel", "Modifier" } });
        if (arg->TypeMod)
            AddRelationship(name, AddNode({ { "label", (string)arg->TypeMod->Text() }, { "color", "blue" } }), { { "headlabel", "Type Modifier" } });
        auto vn = AddNode({ { "label", (string)arg->Name }, { "color", "grey" } });
        AddRelationship(name, vn, { { "headlabel", "Name" } });
        if (auto type = arg->Type)
            AddRelationship(name, Dispatch(type), { { "headlabel", "Type" } });
        if (auto expr = arg->InitializingExpression)
            AddRelationship(name, Dispatch(expr), { { "headlabel", "Value" } });
        return name;
    }

    string_view GraphvizPlotter::Visit(PropertyDeclaration* arg)
    {
        auto name = AddNode({ { "label", "Property Declaration" }, { "color", "yellow" } });
        if (arg->DeclarationToken)
            AddRelationship(name, AddNode({ { "label", (string)arg->DeclarationToken->Text() }, { "color", "purple" } }), { { "headlabel", "Modifier" } });
        auto vn = AddNode({ { "label", (string)arg->Name }, { "color", "grey" } });
        AddRelationship(name, vn, { { "headlabel", "Name" } });
        if (auto type = arg->Type)
            AddRelationship(name, Dispatch(type), { { "headlabel", "Type" } });
        if (auto expr = arg->InitializingExpression)
            AddRelationship(name, Dispatch(expr), { { "headlabel", "Value" } });
        if (arg->Getter)
            AddRelationship(name, Dispatch(arg->Getter), { { "headlabel", "Getter" } });
        if (arg->Setter)
            AddRelationship(name, Dispatch(arg->Setter), { { "headlabel", "Setter" } });
        return name;
    }

    string_view GraphvizPlotter::Visit(ClassDeclaration* arg)
    {
        auto name = AddNode({ { "label", "class '{}'"_f(arg->Name) }, { "color", "green" } });
        auto nn = AddNode({ { "label", (string)arg->Name }, { "color", "grey" } });
        auto sn = AddNode({ { "label", "Statements" } });
        AddRelationship(name, nn, { { "headlabel", "Name" } });
        AddRelationship(name, sn, {  });
        for (auto statement : arg->Statements)
            AddRelationship(sn, Dispatch(statement));
        return name;
    }

    string_view GraphvizPlotter::Visit(FunctionDeclaration* arg)
    {
        auto name = AddNode({ { "label", "Function '{}'"_f(arg->Name) }, { "color", "green" } });
        auto nn = AddNode({ { "label", (string)arg->Name }, { "color", "grey" } });
        AddRelationship(name, nn, { { "headlabel", "Name" } });
        auto params = AddNode({ { "label", "Parameters" } });
        AddRelationship(name, params);
        if (!arg->Parameters.empty())
        {
            for (auto param : arg->Parameters)
            {
                auto pn = Visit(param);
                AddRelationship(params, pn);
            }
            if (arg->IsVariadic)
                AddRelationship(params, AddNode({ { "label", "..." } }));
        }
        if (auto t = arg->ReturnType)
            AddRelationship(name, AddNode({ { "label", t->ToString() } }), { { "headlabel", "Return Type" } });
        if (!arg->Statements.empty())
        {
            auto sta = AddNode({ { "label", "Statements" } });
            AddRelationship(name, sta);
            for (auto statement : arg->Statements)
                AddRelationship(sta, Dispatch(statement));
        }
        return name;
    }

    string_view GraphvizPlotter::Visit(AttributeDeclaration* arg)
    {
        auto name = AddNode({ { "label", "Attribute" }, { "color", "green" } });
        for (auto expr : arg->Expressions)
            AddRelationship(name, Dispatch(expr), { { "headlabel", "Name" } });
        return name;
    }

    string_view GraphvizPlotter::Visit(ConstructorDeclaration* arg)
    {
        auto name = AddNode({ { "label", "{}this({})"_f(arg->IsDestructor ? "~" : "", join(NamedIndexes(arg->Parameters.size()), ", ")) }, { "color", "green" } });
        if (!arg->Parameters.empty())
        {
            auto params = AddNode({ { "label", "Parameters" } });
            AddRelationship(name, params);
            for (auto param : arg->Parameters)
            {
                auto pn = Visit(param);
                AddRelationship(params, pn);
            }
        }
        if (!arg->FieldInitializers.empty())
        {
            auto inits = AddNode({ { "label", "Field Initializers" } });
            AddRelationship(name, inits);
            for (auto init : arg->FieldInitializers)
            {
                auto field = AddNode({ { "label", "{}({})"_f(init->Field->Text(), join(irange(init->Arguments.size()) | vs::transform([](auto i) { return "${}"_f(i); }), ", ")) } });
                AddRelationship(inits, field);
                auto i = 0;
                for (auto v : init->Arguments)
                    AddRelationship(field, Dispatch(v), { { "headlabel", "${}"_f(i++) } });
            }
        }
        if (!arg->Statements.empty())
        {
            auto sta = AddNode({ { "label", "Statements" } });
            AddRelationship(name, sta);
            for (auto statement : arg->Statements)
                AddRelationship(sta, Dispatch(statement));
        }
        return name;
    }

    string_view GraphvizPlotter::Visit(TernaryExpression* arg)
    {
        auto name = AddNode({ { "label", "$0 ? $1 : $2" }, { "color", "green" } });
        AddRelationship(name, Dispatch(arg->Condition), { { "headlabel", "Condition" } });
        AddRelationship(name, Dispatch(arg->TrueExpression), { { "headlabel", "True" } });
        AddRelationship(name, Dispatch(arg->FalseExpression), { { "headlabel", "False" } });
        return name;
    }

    string_view GraphvizPlotter::Visit(OperatorDeclaration* arg)
    {
        auto nn = Dispatch<string>(arg->Operator,
            [&](LParen*) { return "operator[]"; },
            [&](LBracket*) { return "operator()"; },
            [&](Token*) { return "operator{}"_f(arg->Operator->Text()); }
        );
        auto name = AddNode({ { "label", nn }, { "color", "blue" } });
        auto params = AddNode({ { "label", "Parameters" }, { "color", "grey" } });
        AddRelationship(name, params);
        if (!arg->Parameters.empty())
        {
            for (auto param : arg->Parameters)
            {
                auto pn = Visit(param);
                AddRelationship(params, pn);
            }
            //if (arg->IsVariadic)
            //    AddRelationship(params, AddNode({ { "label", "..." } }));
        }
        if (auto t = arg->ReturnType)
            AddRelationship(name, AddNode({ { "label", t->ToString() } }), { { "headlabel", "Return Type" } });
        if (!arg->Statements.empty())
        {
            auto sta = AddNode({ { "label", "Statements" } });
            AddRelationship(name, sta);
            for (auto statement : arg->Statements)
                AddRelationship(sta, Dispatch(statement));
        }
        return name;
    }

    string_view GraphvizPlotter::Visit(MemberAccessExpression* arg)
    {
        if (auto ie = arg->Owner->As<IdentifierExpression>())
            return AddNode({ { "label", "{}.{}"_f(ie->Identifier, arg->Member->Identifier) } });
        auto name = AddNode({ { "label", "$0.{}"_f(arg->Member->Identifier) } });
        AddRelationship(name, Dispatch(arg->Owner), { { "headlabel", "Owner" } });
        AddRelationship(name, Dispatch(arg->Member), { { "headlabel", "Member" } });
        return name;
    }

    string_view GraphvizPlotter::Visit(NewExpression* arg)
    {
        string str = "new T";
        if (!arg->Dimension.empty())
            str += "[{}]"_f(join(NamedIndexes(arg->Dimension.size()), ", "));
        if (!arg->Arguments.empty())
            str += "({})"_f(join(NamedIndexes(arg->Arguments.size()), ", "));
        auto name = AddNode({ { "label", move(str) } });
        AddRelationship(name, Dispatch(arg->Type), { { "headlabel", "Type" } });
        switch (arg->Arguments.size())
        {
        case 0: break;
        case 1:
            AddRelationship(name, Dispatch(arg->Arguments[0]), { { "headlabel", "$0" } });
        break;
        default:
        {
            auto args = AddNode({ { "label", "Arguments" } });
            AddRelationship(name, args);
            auto i = 0;
            for (auto v : arg->Arguments)
                AddRelationship(args, Dispatch(v), { { "headlabel", "${}"_f(i++) } });
        }break;
        }
        switch (arg->Dimension.size())
        {
        case 0: break;
        case 1:
            AddRelationship(name, Dispatch(arg->Dimension[0]), { { "headlabel", "$0" } });
        break;
        default:
        {
            auto args = AddNode({ { "label", "Dimension" } });
            AddRelationship(name, args);
            auto i = 0;
            for (auto v : arg->Dimension)
                AddRelationship(args, Dispatch(v), { { "headlabel", "${}"_f(i++) } });
        }break;
        }
        return name;
    }

    string_view GraphvizPlotter::Visit(NamedOperatorExpression* arg)
    {
        auto name = AddNode({ { "label", "NamedOperatorExpression" } });
        AddRelationship(name, AddNode({ { "label", (string)arg->Token->Text() } }), { { "headlabel", "Operator" } });
        auto n = arg->Token->Is<Return>() && !arg->Argument ?
            AddNode({ { "label", "(void)" } }) :
            Dispatch(arg->Argument);
        AddRelationship(name, n, { { "headlabel", "Argument" } });
        return name;
    }

    string_view GraphvizPlotter::Visit(NamedTypeOperatorExpression* arg)
    {
        auto name = AddNode({ { "label", "NamedTypeOperatorExpression" } });
        AddRelationship(name, AddNode({ { "label", (string)arg->Token->Text() } }), { { "headlabel", "Operator" } });
        AddRelationship(name, Dispatch(arg->Type), { { "headlabel", "Type" } });
        return name;
    }

}
