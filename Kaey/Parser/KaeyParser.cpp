#include "KaeyParser.hpp"
#include "Utility.hpp"

using std::pair;
using std::tuple;
using std::vector;

using namespace Kaey::Lexer;

namespace Kaey::Parser
{
    namespace
    {
        using svmatch_results = std::match_results<string_view::const_iterator>;

        template<class BidirIt, class Traits, class CharT, class UnaryFunction>
        std::basic_string<CharT> regex_replace(BidirIt first, BidirIt last, const std::basic_regex<CharT, Traits>& re, UnaryFunction f)
        {
            std::basic_string<CharT> s;

            typename std::match_results<BidirIt>::difference_type positionOfLastMatch = 0;
            auto endOfLastMatch = first;

            auto callback = [&](const std::match_results<BidirIt>& match)
            {
                auto positionOfThisMatch = match.position(0);
                auto diff = positionOfThisMatch - positionOfLastMatch;

                auto startOfThisMatch = endOfLastMatch;
                std::advance(startOfThisMatch, diff);

                s.append(endOfLastMatch, startOfThisMatch);
                s.append(f(match));

                auto lengthOfMatch = match.length(0);

                positionOfLastMatch = positionOfThisMatch + lengthOfMatch;

                endOfLastMatch = startOfThisMatch;
                std::advance(endOfLastMatch, lengthOfMatch);
            };

            std::regex_iterator<BidirIt> begin(first, last, re), end;
            std::for_each(begin, end, callback);

            s.append(endOfLastMatch, last);

            return s;
        }

        template<class Traits, class CharT, class UnaryFunction>
        auto regex_replace(string_view s, const std::basic_regex<CharT, Traits>& re, UnaryFunction f)
        {
            return regex_replace(s.cbegin(), s.cend(), re, f);
        }

        tuple<int, bool, uint64_t> ParseInt(const char* str, int radix)
        {
            auto& e = errno;
            e = 0;
            auto v = (uint64_t)std::strtol(str, nullptr, radix);
            if (!e) return { 32, true, v };
            e = 0;
            v = (uint64_t)std::strtoul(str, nullptr, radix);
            if (!e) return { 32, false, v };
            e = 0;
            v = (uint64_t)std::strtoll(str, nullptr, radix);
            if (!e) return { 64, true, v };
            e = 0;
            v = (uint64_t)std::strtoull(str, nullptr, radix);
            if (!e) return { 64, false, v };
            throw std::runtime_error("");
        }

        tuple<int, bool, uint64_t> ParseInt(RegexToken* tk)
        {
            vector<string> r;
            for (auto& s : tk->Result() | vs::drop(1)) if (s.matched)
                r.emplace_back((string)s);
            auto radix = 0;
            switch (tk->KindId())
            {
            case BinaryIntegerLiteral::Id:
                r[0].erase(0, 2);
                radix = 2;
                [[fallthrough]];
            case HexadecimalIntegerLiteral::Id:
            case IntegerLiteral::Id:
            case OctalIntegerLiteral::Id:
            {
                auto result = ParseInt(r[0].data(), radix);
                switch (r.size())
                {
                case 3:
                    get<0>(result) = std::strtol(r[2].data(), nullptr, 10);
                    [[fallthrough]];
                case 2:
                    get<1>(result) = r[1] == "i";
                default:
                    return result;
                }
            }
            default:
                throw std::runtime_error("");
            }
        }

        float ParseFloat(Token* tk)
        {
            auto str = (string)tk->Text();
            switch (tk->KindId())
            {
            case FloatingPointLiteral::Id: return std::stof(str);
            }
            throw std::runtime_error("");
        }

        vector<char> ParseString(RegexToken* tk)
        {
            using namespace Kaey;
            switch (tk->KindId())
            {
            case StringLiteral::Id:
            {
                auto sv = tk->Text();
                sv.remove_prefix(1);
                sv.remove_suffix(1);
                auto r = std::regex(R"(\\([0-7]{1,3}))");
                auto str = regex_replace(sv, r, [](const svmatch_results& match) -> string
                {
                    static constexpr auto max = 0xFF;
                    auto str = match[1].str();
                    auto d = std::stoi(str, nullptr, 8);
                    if (d > max) throw std::runtime_error("The value of an octal escape sequence may not surpass {}, value was {} with format {}."_f(max, d, str));
                    str.resize(1);
                    str[0] = char(d);
                    return str;
                });
                r = std::regex(R"(\\[Xx]([0-9A-F]{1,2}))");
                str = regex_replace(str, r, [](const svmatch_results& match) -> string
                {
                    auto str = match[1].str();
                    auto d = std::stoi(str, nullptr, 16);
                    str.resize(1);
                    str[0] = char(d);
                    return str;
                });
                r = std::regex(R"(\\(.))");
                str = regex_replace(str, r, [](const svmatch_results& match) -> string
                {
                    auto c = match[1].str()[0];
                    switch (c)
                    {
                    case '\'': return "\'";
                    case '\"': return "\"";
                    case '\\': return "\\";
                    case 'a': return "\a";
                    case 'b': return "\b";
                    case 'f': return "\f";
                    case 'n': return "\n";
                    case 'r': return "\r";
                    case 't': return "\t";
                    case 'v': return "\v";
                    }
                    throw std::runtime_error("Invalid escape sequence: {}"_f(c));
                });
                auto v = str | to_vector;
                v.emplace_back('\0');
                return v;
            }
            case RawStringLiteral::Id:
                return tk->Value() | to_vector;
            default:
                assert(false);
                throw std::runtime_error("Invalid Token!");
            }
        }

        bool IsBinaryOperator(const Token* tk)
        {
            switch (tk->KindId())
            {
            case DoubleQuestion::Id:

            case Plus::Id:
            case Minus::Id:
            case Star::Id:
            case Slash::Id:
            case Modulo::Id:

            case Pipe::Id:
            case Ampersand::Id:
            case Caret::Id:

            case And::Id:
            case Or::Id:

            case LeftShift::Id:
            case RightShift::Id:

            case Assign::Id:

            case LeftShiftAssign::Id:
            case RightShiftAssign::Id:

            case AmpersandAssign::Id:
            case PipeAssign::Id:
            case CaretAssign::Id:

            case PlusAssign::Id:
            case MinusAssign::Id:
            case StarAssign::Id:
            case SlashAssign::Id:
            case ModuloAssign::Id:

            case Equal::Id:
            case NotEqual::Id:

            case Less::Id:
            case LessEqual::Id:
            case Greater::Id:
            case GreaterEqual::Id:

                return true;
            default:
                return false;
            }
        }

    }

    BooleanExpression::BooleanExpression(bool value): Value(value)
    {
        
    }

    string BooleanExpression::ToString() const
    {
        return Value ? "true" : "false";
    }

    IntegerExpression::IntegerExpression(RegexToken* token) : Token(token)
    {
        std::tie(Bits, IsSigned, Value) = ParseInt(token);
    }

    string IntegerExpression::ToString() const
    {
        return (string)Token->Text();
    }

    FloatingExpression::FloatingExpression(RegexToken* token) : Token(token), Value(ParseFloat(token))
    {
        
    }

    string FloatingExpression::ToString() const
    {
        return (string)Token->Text();
    }

    UnaryOperatorExpression::UnaryOperatorExpression(Token* operation, Expression* operand, bool isPost): Operation(operation), Operand(operand), IsSuffix(isPost)
    {
        
    }

    string UnaryOperatorExpression::ToString() const
    {
        return IsSuffix ? "{}{}"_f(Operand->ToString(), Operation->Text()) : "{}{}"_f(Operation->Text(), Operand->ToString());
    }

    BinaryOperatorExpression::BinaryOperatorExpression(Expression* left, OperatorToken* operation, Expression* right): Left(left), Operation(operation), Right(right)
    {
        
    }

    string BinaryOperatorExpression::ToString() const
    {
        return "{} {} {}"_f(Left->ToString(), Operation->Text(), Right->ToString());
    }

    CallExpression::CallExpression(Expression* callee, vector<Expression*> arguments): Callee(callee), Arguments(move(arguments))
    {
        
    }

    string CallExpression::ToString() const
    {
        return "{}({})"_f(Callee->ToString(), join(Arguments | vs::transform(mem_fn(&Expression::ToString)), ", "));
    }

    IdentifierExpression::IdentifierExpression(string identifier): Identifier(move(identifier))
    {
        
    }

    string IdentifierExpression::ToString() const
    {
        return Identifier;
    }

    ParenthisedExpression::ParenthisedExpression(Expression* underlyingExpression): UnderlyingExpression(underlyingExpression)
    {
        
    }

    string ParenthisedExpression::ToString() const
    {
        return "({})"_f(UnderlyingExpression->ToString());
    }

    TupleExpression::TupleExpression(vector<Expression*> expressions): Expressions(move(expressions))
    {
        assert(!Expressions.empty());
    }

    string TupleExpression::ToString() const
    {
        return Expressions.size() > 1 ? "({})"_f(join(Expressions | vs::transform(std::mem_fn(&Expression::ToString)), ", ")) : "({},)"_f(Expressions[0]->ToString());
    }

    StringLiteralExpression::StringLiteralExpression(RegexToken* token) : Token(token), Value(ParseString(token))
    {
        
    }

    string StringLiteralExpression::ToString() const
    {
        return Value.data();
    }

    SubscriptOperator::SubscriptOperator(Expression* callee, vector<Expression*> arguments): Callee(callee), Arguments(move(arguments))
    {
        
    }

    string SubscriptOperator::ToString() const
    {
        return "{}[{}]"_f(Callee->ToString(), join(Arguments | vs::transform(mem_fn(&Expression::ToString)), ", "));
    }

    TypeCastExpression::TypeCastExpression(Expression* convertedExpression, Token* convertionType, BaseType* type) : ConvertedExpression(convertedExpression), ConvertionType(convertionType), Type(type)
    {
        
    }

    string TypeCastExpression::ToString() const
    {
        return "{} {} {}"_f(ConvertedExpression->ToString(), ConvertionType->Text(), Type->ToString());
    }

    TernaryExpression::TernaryExpression(Expression* condition, Expression* trueExpression, Expression* falseExpression) : Condition(condition), TrueExpression(trueExpression), FalseExpression(falseExpression)
    {

    }

    string TernaryExpression::ToString() const
    {
        return Condition->ToString() + " ? " + TrueExpression->ToString() + " : " + FalseExpression->ToString();
    }

    MemberAccessExpression::MemberAccessExpression(Expression* owner, IdentifierExpression* memberIdentifier) : Owner(owner), Member(memberIdentifier)
    {

    }

    string MemberAccessExpression::ToString() const
    {
        return Owner->ToString() + "." + Member->ToString();
    }

    NewExpression::NewExpression(BaseType* type, vector<Expression*> dimension, vector<Expression*> arguments) : Type(type), Dimension(move(dimension)), Arguments(move(arguments))
    {

    }

    string NewExpression::ToString() const
    {
        //return "new {}({})"_f(type->ToString(), join(Arguments | ));
        return { };
    }

    NamedOperatorExpression::NamedOperatorExpression(Lexer::Token* token, Expression* argument) : Token(token), Argument(argument)
    {

    }

    string NamedOperatorExpression::ToString() const
    {
        return { };
    }

    NamedTypeOperatorExpression::NamedTypeOperatorExpression(Lexer::Token* token, BaseType* type) : Token(token), Type(type)
    {

    }

    string NamedTypeOperatorExpression::ToString() const
    {
        return { };
    }

    IdentifierType::IdentifierType(string identifier): Identifier(move(identifier))
    {
        
    }

    string IdentifierType::ToString() const
    {
        return Identifier;
    }

    DecoratedType::DecoratedType(BaseType* underlyingType, Token* decorator): UnderlyingType(underlyingType), Decorator(decorator)
    {
        assert(UnderlyingType);
    }

    string DecoratedType::ToString() const
    {
        return "{}{}"_f(UnderlyingType->ToString(), Decorator->Text());
    }

    TupleType::TupleType(vector<BaseType*> types): Types(move(types))
    {
        assert(!Types.empty());
    }

    string TupleType::ToString() const
    {
        return "({}{}"_f(join(Types | vs::transform(mem_fn(&BaseType::ToString)), ", "), Types.size() > 1 ? ")" : ",)");
    }

    ArrayType::ArrayType(BaseType* underlyingType, vector<Expression*> lengthExpressions) : UnderlyingType(underlyingType), LengthExpressions(move(lengthExpressions))
    {
        
    }

    string ArrayType::ToString() const
    {
        return "{}[{}]"_f(UnderlyingType->ToString(), join(LengthExpressions | vs::transform([](auto ptr) { return ptr ? ptr->ToString() : ""; }), ", "));
    }

    VariantType::VariantType(vector<BaseType*> underlyingTypes) : UnderlyingTypes(move(underlyingTypes))
    {

    }

    string VariantType::ToString() const
    {
        return "{}"_f(join(UnderlyingTypes | vs::transform([](auto ptr) { return ptr->ToString(); }), " | "));
    }

    TypeParser::TypeParser(ExpressionParser* expressionParser): expressionParser(expressionParser)
    {
        
    }

    BaseType* TypeParser::Parse(TokenStream& stream)
    {
        return ParseUnaryType(stream);
    }

    BaseType* TypeParser::ParseUnaryType(TokenStream& stream)
    {
        auto result = Dispatch<BaseType*>(stream.Peek(),
            [&](Identifier* tk)
            {
                stream.Consume();
                return Create<IdentifierType>((string)tk->Text());
            },
            [&](LParen*) -> BaseType*
            {
                stream.Consume();
                auto [types, tks] = ParseWhile<Comma>(stream);
                auto rp = stream.Consume();
                assert(rp->Is<RParen>());
                if (types.size() == 1 && tks.empty())
                    return types[0];
                return Create<TupleType>(move(types));
            }
        );
        if (!result) return nullptr;
        while (auto post = Dispatch<BaseType*>(stream.Peek(),
                [&](Star* tk)
                {
                    stream.Consume();
                    return Create<DecoratedType>(result, tk);
                },
                [&](Ampersand* tk)
                {
                    stream.Consume();
                    return Create<DecoratedType>(result, tk);
                },
                [&](Caret* tk)
                {
                    stream.Consume();
                    return Create<DecoratedType>(result, tk);
                },
                [&](LBracket*)
                {
                    stream.Consume();
                    auto [exprs, _] = expressionParser->ParseWhile<Comma>(stream, true);
                    auto rb = stream.Consume();
                    assert(rb->Is<RBracket>());
                    return Create<ArrayType>(result, move(exprs));
                },
                [&](Pipe*)
                {
                    stream.Consume();
                    auto [types, _] = ParseWhile<Pipe>(stream);
                    types.emplace(types.begin(), result);
                    for (size_t i = 0; i < types.size(); ++i)
                    {
                        auto ty = types[i];
                        if (auto v = ty->As<VariantType>())
                        {
                            types.erase(types.begin() + i);
                            for (auto t : v->UnderlyingTypes)
                                types.emplace_back(t);
                            --i;
                        }
                    }
                    return Create<VariantType>(move(types));
                },
                [&](Question* tk) -> DecoratedType*
                {
                    //if (auto dt = result->As<DecoratedType>(); dt && dt->Decorator->Is<Question>())
                    //    return nullptr;
                    stream.Consume();
                    return Create<DecoratedType>(result, tk);
                }
            ))
            result = post;
        return result;
    }
    
    ExpressionParser::ExpressionParser(TypeParser* typeParser) : typeParser(typeParser)
    {
        
    }

    Expression* ExpressionParser::Parse(TokenStream& stream)
    {
        return ParseBinaryExpression(stream, ParseUnaryExpression(stream));
    }

    Expression* ExpressionParser::ParseUnaryExpression(TokenStream& stream, Token* unaryToken)
    {
        using namespace Lexer;
        auto result = Dispatch<Expression*>(stream.Peek(),
            [&](True*)
            {
                stream.Consume();
                return Create<BooleanExpression>(true);
            },
            [&](False*)
            {
                stream.Consume();
                return Create<BooleanExpression>(false);
            },
            [&](This* tk)
            {
                stream.Consume();
                return Create<IdentifierExpression>((string)tk->Text());
            },
            [&](Identifier* tk)
            {
                stream.Consume();
                return Create<IdentifierExpression>((string)tk->Text());
            },
            [&](IntegerLiteral* tk)
            {
                stream.Consume();
                return Create<IntegerExpression>(tk);
            },
            [&](HexadecimalIntegerLiteral* tk)
            {
                stream.Consume();
                return Create<IntegerExpression>(tk);
            },
            [&](BinaryIntegerLiteral* tk)
            {
                stream.Consume();
                return Create<IntegerExpression>(tk);
            },
            [&](OctalIntegerLiteral* tk)
            {
                stream.Consume();
                return Create<IntegerExpression>(tk);
            },
            [&](Plus* tk)
            {
                stream.Consume();
                auto expr = ParseUnaryExpression(stream, tk);
                return expr ? Create<UnaryOperatorExpression>(tk, expr) : throw UnexpectedTokenException(stream.Peek());
            },
            [&](Minus* tk)
            {
                stream.Consume();
                auto expr = ParseUnaryExpression(stream, tk);
                return expr ? Create<UnaryOperatorExpression>(tk, expr) : throw UnexpectedTokenException(stream.Peek());
            },
            [&](Star* tk)
            {
                stream.Consume();
                auto expr = ParseUnaryExpression(stream, tk);
                return expr ? Create<UnaryOperatorExpression>(tk, expr) : throw UnexpectedTokenException(stream.Peek());
            },
            [&](Exclamation* tk)
            {
                stream.Consume();
                auto expr = ParseUnaryExpression(stream, tk);
                return expr ? Create<UnaryOperatorExpression>(tk, expr) : throw UnexpectedTokenException(stream.Peek());
            },
            [&](Increment* tk)
            {
                stream.Consume();
                auto expr = ParseUnaryExpression(stream, tk);
                return expr ? Create<UnaryOperatorExpression>(tk, expr) : throw UnexpectedTokenException(stream.Peek());
            },
            [&](Decrement* tk)
            {
                stream.Consume();
                auto expr = ParseUnaryExpression(stream, tk);
                return expr ? Create<UnaryOperatorExpression>(tk, expr) : throw UnexpectedTokenException(stream.Peek());
            },
            [&](Ampersand* tk)
            {
                stream.Consume();
                auto expr = ParseUnaryExpression(stream, tk);
                return expr ? Create<UnaryOperatorExpression>(tk, expr) : throw UnexpectedTokenException(stream.Peek());
            },
            [&](LParen*) -> Expression*
            {
                stream.Consume();
                vector<Expression*> exprs;
                while (true)
                {
                    auto expr = Parse(stream);
                    assert(expr);
                    exprs.emplace_back(expr);
                    auto next = stream.Peek();
                    if (next->Is<RParen>())
                    {
                        stream.Consume();
                        if (exprs.size() == 1)
                            return Create<ParenthisedExpression>(exprs[0]);
                        break;
                    }
                    assert(next->Is<Comma>());
                    stream.Consume();
                    if (stream.Peek()->Is<RParen>())
                    {
                        assert(exprs.size() == 1);
                        stream.Consume();
                        break;
                    }
                }
                return Create<TupleExpression>(move(exprs));
            },
            [&](StringLiteral* lit)
            {
                stream.Consume();
                return Create<StringLiteralExpression>(lit);
            },
            [&](RawStringLiteral* lit)
            {
                stream.Consume();
                return Create<StringLiteralExpression>(lit);
            },
            [&](FloatingPointLiteral* tk)
            {
                stream.Consume();
                return Create<FloatingExpression>(tk);
            },
            [&](New* tk)
            {
                stream.Consume();
                auto type = typeParser->Parse(stream);
                assert(type);
                vector<Expression*> dim, args;
                if (auto at = type->As<ArrayType>(); at && (at->LengthExpressions.size() != 1 || at->LengthExpressions[0] != nullptr))
                {
                    dim = at->LengthExpressions;
                    type = at->UnderlyingType;
                }
                else if (auto lb = stream.Peek(); lb->Is<LBracket>()) //Should never happen.
                {
                    stream.Consume();
                    auto [v, _] = ParseWhile<Comma>(stream);
                    swap(dim, v);
                    auto rb = stream.Consume();
                    assert(rb->Is<RBracket>());
                }
                if (auto lp = stream.Peek(); lp->Is<LParen>())
                {
                    stream.Consume();
                    auto [v, _] = ParseWhile<Comma>(stream);
                    swap(args, v);
                    auto rp = stream.Consume();
                    assert(rp->Is<RParen>());
                }
                return Create<NewExpression>(type, move(dim), move(args));
            },
            [&](Delete* tk)
            {
                stream.Consume();
                auto arg = Parse(stream);
                assert(arg);
                return Create<NamedOperatorExpression>(tk, arg);
            },
            [&](Throw* tk)
            {
                stream.Consume();
                auto arg = Parse(stream);
                assert(arg);
                return Create<NamedOperatorExpression>(tk, arg);
            },
            [&](Return* tk)
            {
                stream.Consume();
                auto arg = Parse(stream);
                return Create<NamedOperatorExpression>(tk, arg);
            },
            [&](SizeOf* tk)
            {
                stream.Consume();
                auto type = typeParser->Parse(stream);
                assert(type);
                if (auto dt = type->As<DecoratedType>(); dt && dt->Decorator->Is<Question>())
                {
                    auto pos = stream.CurrentPosition();
                    if (auto e = Parse(stream); e && stream.Peek()->Is<Colon>())
                    {
                        type = dt->UnderlyingType;
                        typeParser->Destroy(dt);
                        Destroy(e);
                        stream.SetCurrentPosition(pos - 1);
                    }
                }
                return Create<NamedTypeOperatorExpression>(tk, type);
            },
            [&](AlignOf* tk)
            {
                stream.Consume();
                auto type = typeParser->Parse(stream);
                assert(type);
                if (auto dt = type->As<DecoratedType>(); dt && dt->Decorator->Is<Question>())
                {
                    auto pos = stream.CurrentPosition();
                    if (auto e = Parse(stream); e && stream.Peek()->Is<Colon>())
                    {
                        type = dt->UnderlyingType;
                        typeParser->Destroy(dt);
                        Destroy(e);
                        stream.SetCurrentPosition(pos - 1);
                    }
                }
                return Create<NamedTypeOperatorExpression>(tk, type);
            }
        );
        if (!result) return nullptr;
        auto parseTypeCast = [&](auto tk) -> Expression*
        {
            stream.Consume();
            auto type = typeParser->Parse(stream);
            assert(type);
            if (auto dt = type->As<DecoratedType>(); dt && dt->Decorator->Is<Question>())
            {
                auto pos = stream.CurrentPosition();
                auto e = Parse(stream);
                if (e && stream.Peek()->Is<Colon>())
                {
                    type = dt->UnderlyingType;
                    typeParser->Destroy(dt);
                    Destroy(e);
                    stream.SetCurrentPosition(pos - 1);
                }
            }
            return Create<TypeCastExpression>(result, tk, type);
        };
        while (auto post = Dispatch<Expression*>(stream.Peek(),
            [&](Increment* tk)
            {
                stream.Consume();
                return Create<UnaryOperatorExpression>(tk, result, true);
            },
            [&](Decrement* tk)
            {
                stream.Consume();
                return Create<UnaryOperatorExpression>(tk, result, true);
            },
            [&](LParen*)
            {
                stream.Consume();
                Token* last;
                vector<Expression*> args;
                do
                {
                    auto e = Parse(stream);
                    last = stream.Consume();
                    if (!last->Is<Comma>() && !last->Is<RParen>())
                        throw UnexpectedTokenException(last);
                    if (e) args.emplace_back(e);
                } while (last->Is<Comma>());
                return Create<CallExpression>(result, move(args));
            },
            [&](Ellipsis* tk)
            {
                stream.Consume();
                return Create<UnaryOperatorExpression>(tk, result);
            },
            [&](LBracket*)
            {
                stream.Consume();
                Token* last;
                vector<Expression*> args;
                do
                {
                    auto e = Parse(stream);
                    last = stream.Consume();
                    if (!last->Is<Comma>() && !last->Is<RBracket>())
                        throw UnexpectedTokenException(last);
                    if (e) args.emplace_back(e);
                } while (last->Is<Comma>());
                return Create<SubscriptOperator>(result, move(args));
            },
            [&](Dot* tk)
            {
                stream.Consume();
                auto mn = stream.Consume();
                assert(mn->Is<Identifier>());
                auto member = Create<IdentifierExpression>((string)mn->Text());
                return Create<MemberAccessExpression>(result, member);
            },
            [&](Is* tk) { return parseTypeCast(tk); },
            [&](As* tk) { return parseTypeCast(tk); },
            [&](Question* tk)
            {
                stream.Consume();
                auto expr = Parse(stream);
                assert(expr);
                auto colon = stream.Consume();
                assert(colon->Is<Colon>());
                auto falseCond = Parse(stream);
                if (!falseCond)
                    throw UnexpectedTokenException(stream.Peek());
                return Create<TernaryExpression>(result, expr, falseCond);
            }
            ))
            result = post;
        return result;
    }

    Expression* ExpressionParser::ParseBinaryExpression(TokenStream& stream, Expression* lhs, const OperatorToken* lastOp)
    {
        if (!lhs) return nullptr;
        while (true)
        {
            auto op = stream.Peek()->AsOperator();
            if (!op || !op->IsBinaryOperator() || lastOp && lastOp <= op)
                return lhs;
            stream.Consume();
            auto expr = ParseUnaryExpression(stream);
            if (!expr)
                throw UnexpectedTokenException(stream.Peek());
            for (auto next = stream.Peek(); IsBinaryOperator(next) && (op > next || op->IsRightToLeft()); next = stream.Peek())
            {
                auto e = ParseBinaryExpression(stream, expr, op);
                if (!e) throw UnexpectedTokenException(stream.Peek());
                expr = e;
            }

            lhs = Create<BinaryOperatorExpression>(lhs, op, expr);
        }
        return lhs;
    }

    Statement::Statement(vector<AttributeDeclaration*> attributes) : Attributes(move(attributes))
    {
        
    }

    FunctionDeclaration::FunctionDeclaration(string name, vector<VariableDeclaration*> parameters, BaseType* returnType, bool isVariadic, vector<Statement*> statements, Expression* lambdaReturn) :
        Name(move(name)), Parameters(move(parameters)), ReturnType(returnType),
        IsVariadic(isVariadic), LambdaReturn(lambdaReturn), Statements(move(statements))
    {

    }
    
    void FunctionDeclaration::Print(ostream& os, string ident)
    {

    }

    void OperatorDeclaration::Print(ostream& os, string ident)
    {

    }

    ConstructorDeclaration::ConstructorDeclaration(vector<VariableDeclaration*> parameters, vector<FieldInitializerStatement*> fieldInitializers, vector<Statement*> statements, bool isDestructor, bool isLambdaReturn) :
        Parameters(move(parameters)), FieldInitializers(move(fieldInitializers)), Statements(move(statements)), IsDestructor(isDestructor), IsLambdaReturn(isLambdaReturn)
    {

    }

    void ConstructorDeclaration::Print(ostream& os, string ident)
    {
        
    }

    FieldInitializerStatement::FieldInitializerStatement(Token* field, vector<Expression*> arguments) : Field(field), Arguments(move(arguments))
    {
        
    }

    void FieldInitializerStatement::Print(ostream& os, string ident)
    {
        
    }

    AttributeDeclaration::AttributeDeclaration(vector<Expression*> expressions) : Expressions(move(expressions))
    {
        
    }

    void AttributeDeclaration::Print(ostream& os, string ident)
    {
        
    }

    SwitchStatement::SwitchStatement(Expression* condition, vector<pair<Expression*, Statement*>> cases) : Condition(condition), Cases(move(cases))
    {

    }

    void SwitchStatement::Print(ostream& os, string ident)
    {

    }

    StatementParser::StatementParser() : expressionParser(&typeParser), typeParser(&expressionParser)
    {
        
    }

    Statement* StatementParser::Parse(TokenStream& stream)
    {
        auto defFn = [&](Token* tk)
        {
            auto expression = expressionParser.Parse(stream);
            if (!expression)
                throw UnexpectedTokenException(tk);
            auto semi = stream.Consume();
            assert(semi->Is<Semi>());
            return Create<ExpressionStatement>(expression);
        };
        return Dispatch<Statement*>(stream.Peek(),
            [&](If*) { return ParseIf(stream); },
            [&](LBrace*) { return ParseBlock(stream); },
            [&](While*) { return ParseWhileStatement(stream); },
            [&](For*) { return ParseForStatement(stream); },
            [&](Switch*) { return ParseSwitchStatement(stream); },
            [&](Var*) { return ParseVariableDeclaration(stream); },
            [&](Val*) { return ParseVariableDeclaration(stream); },
            [&](Class*) { return ParseClassDeclaration(stream); },
            [&](LBracket*) { return ParseFunctionDeclaration(stream); },
            [&](HashTag*)
            {
                vector attrs{ ParseAttributeDeclaration(stream) };
                while (true)
                {
                    auto next = Parse(stream);
                    assert(next);
                    if (auto attr = next->As<AttributeDeclaration>())
                    {
                        attrs.emplace_back(attr);
                        continue;
                    }
                    next->Attributes = move(attrs);
                    return next;
                }
            },
            [&](This*) { return ParseConstructor(stream); },
            [&](Tilde* tk) -> Statement*
            {
                auto next = stream.Peek(1);
                if (next->IsNot<This>())
                    return defFn(tk);
                return ParseConstructor(stream);
            },
            [&](Operator*) { return ParseOperatorDeclaration(stream); },
            [&](Eof*) { return &eofStatement; },
            defFn
        );
    }

    ExpressionStatement::ExpressionStatement(Expression* underlyingExpression): UnderlyingExpression(underlyingExpression)
    {
        
    }

    void ExpressionStatement::Print(ostream& os, string ident)
    {
        os << ident << (UnderlyingExpression ? "{};\n"_f(UnderlyingExpression->ToString()) : ";\n");
    }

    BlockStatement::BlockStatement(vector<Statement*> statements) : Statements(move(statements))
    {
        
    }

    void BlockStatement::Print(ostream& os, string ident)
    {
        os << ident << "{\n";
        auto nextIdent = ident + IDENT_SPACE;
        if (!Statements.empty())
            for (auto statement : Statements)
                statement->Print(os, nextIdent);
        else os << "\n";
        os << ident << "}\n";
    }

    IfStatement::IfStatement(Expression* condition, Statement* trueStatement, Statement* falseStatement): Condition(condition), TrueStatement(trueStatement), FalseStatement(falseStatement)
    {
        assert(condition && trueStatement);
    }

    void IfStatement::Print(ostream& os, string ident)
    {
        os << ident << "if (" << Condition->ToString() << ")\n";
        TrueStatement->Print(os, ident);
        if (!FalseStatement) return;
        os << ident << "else ";
        if (FalseStatement->Is<ExpressionStatement>())
            FalseStatement->Print(os);
        else
        {
            os << "\n";
            FalseStatement->Print(os, ident + "  ");
        }
    }

    WhileStatement::WhileStatement(Expression* condition, Statement* loopStatement): Condition(condition), LoopStatement(loopStatement)
    {
        
    }

    void WhileStatement::Print(ostream& os, string ident)
    {
        os << ident << "while (" << Condition->ToString() << ")\n";
        LoopStatement->Print(os, ident);
    }

    ForStatement::ForStatement(Statement* startStatement, Expression* condition, Expression* increment, Statement* loopStatement) :
        StartStatement(startStatement), Condition(condition), Increment(increment), LoopStatement(loopStatement)
    {

    }

    void ForStatement::Print(ostream& os, string ident)
    {
        os << ident << "for (";
        StartStatement->Print(os);
        os << Condition->ToString() << "; ";
        os << Increment->ToString() << ")\n";
        LoopStatement->Print(os, move(ident));
    }

    void EofStatement::Print(ostream& os, string ident)
    {
        assert(false && "'Print' should not be called on 'EofStatement'!");
        throw std::logic_error("'Print' should not be called on 'EofStatement'!");
    }

    VariableDeclarationBase::VariableDeclarationBase(Token* declarationToken, Expression* initializingExpression, vector<AttributeDeclaration*> attributes) :
        Statement(move(attributes)), DeclarationToken(declarationToken), InitializingExpression(initializingExpression)
    {
        
    }

    VariableDeclaration::VariableDeclaration(Token* typeMod, Token* declarationToken, string name, BaseType* type, Expression* initializingExpression) :
        ParentClass(declarationToken, initializingExpression), TypeMod(typeMod), Name(move(name)), Type(type)
    {

    }

    void VariableDeclaration::Print(ostream& os, string ident)
    {
        os << ident << DeclarationToken->Text() << ' ' << Name;
        if (Type)
            os << ": " << Type->ToString();
        if (InitializingExpression)
            os << " = " << InitializingExpression->ToString();
        os << ";\n";
    }

    TupleVariableDeclaration::TupleVariableDeclaration(Token* declarationToken, vector<string> names, TupleType* type, Expression* initializingExpression) :
        ParentClass(declarationToken, initializingExpression), Names(move(names)), Type(type)
    {
        
    }

    void TupleVariableDeclaration::Print(ostream& os, string ident)
    {
        os << ident << DeclarationToken->Text() << " (";
        os << "{}"_f(join(Names, ", ")) << ") ";
        if (Type)
            os << " : " << Type->ToString();
        if (InitializingExpression)
            os << " = " << InitializingExpression->ToString();
        os << ";\n";
    }

    PropertyDeclaration::PropertyDeclaration(Token* declarationToken, string name, BaseType* type, Expression* initializingExpression, Statement* getter, Statement* setter) :
        ParentClass(declarationToken, initializingExpression), Name(move(name)), Type(type), Getter(getter), Setter(setter)
    {

    }

    void PropertyDeclaration::Print(ostream& os, string ident)
    {

    }

    ClassDeclaration::ClassDeclaration(string name, ConstructorDeclaration* defaultConstructor, vector<ClassInheritanceStatement*> inheritances, vector<Statement*> statements) :
        Name(move(name)), DefaultConstructor(defaultConstructor), Inheritances(move(inheritances)), Statements(move(statements))
    {
        
    }

    void ClassDeclaration::Print(ostream& os, string ident)
    {
        os << ident << "class " << Name << '\n';
        os << ident << "{\n";
        auto nextIdent = ident + IDENT_SPACE;
        for (auto statement : Statements)
            statement->Print(os, nextIdent);
        os << ident << "}\n";
    }

    ClassInheritanceStatement::ClassInheritanceStatement(IdentifierType* type, vector<Expression*> arguments) : Type(type), Arguments(move(arguments))
    {
        
    }

    void ClassInheritanceStatement::Print(ostream& os, string ident)
    {

    }

    OperatorDeclaration::OperatorDeclaration(Token* op, vector<VariableDeclaration*> parameters, BaseType* returnType, bool isLambdaReturn, vector<Statement*> statements) :
        Operator(op), Parameters(move(parameters)), ReturnType(returnType),
        IsLambdaReturn(isLambdaReturn), Statements(move(statements))
    {
        
    }

    IfStatement* StatementParser::ParseIf(TokenStream& stream)
    {
        stream.Consume();
        auto lParen = stream.Consume();
        assert(lParen->Is<LParen>());
        auto cond = expressionParser.Parse(stream);
        auto rParen = stream.Consume();
        assert(rParen->Is<RParen>());
        auto trueStatement = Parse(stream);
        if (!stream.Peek()->Is<Else>())
            return Create<IfStatement>(cond, trueStatement);
        stream.Consume();
        return Create<IfStatement>(cond, trueStatement, Parse(stream));
    }

    BlockStatement* StatementParser::ParseBlock(TokenStream& stream)
    {
        auto lbrace = stream.Consume();
        assert(lbrace->Is<LBrace>());
        vector<Statement*> statements;
        while (true)
        {
            if (stream.Peek()->Is<RBrace>())
            {
                stream.Consume();
                return Create<BlockStatement>(move(statements));
            }
            statements.emplace_back(Parse(stream));
        }
    }

    WhileStatement* StatementParser::ParseWhileStatement(TokenStream& stream)
    {
        stream.Consume();
        auto lParen = stream.Consume();
        assert(lParen->Is<LParen>());
        auto cond = expressionParser.Parse(stream);
        auto rParen = stream.Consume();
        assert(rParen->Is<RParen>());
        auto statement = Parse(stream);
        return Create<WhileStatement>(cond, statement);
    }

    ForStatement* StatementParser::ParseForStatement(TokenStream& stream)
    {
        stream.Consume();
        auto lParen = stream.Consume();
        assert(lParen->Is<LParen>());
        //TODO create parse filtering.
        auto init = Parse(stream);
        assert((init->IsOr<VariableDeclaration, ExpressionStatement, TupleVariableDeclaration>()));
        auto cond = expressionParser.Parse(stream);
        auto semi = stream.Consume();
        assert(semi->Is<Semi>());
        auto increment = expressionParser.Parse(stream);
        auto rParen = stream.Consume();
        assert(rParen->Is<RParen>());
        auto statement = Parse(stream);
        return Create<ForStatement>(init, cond, increment, statement);
    }

    SwitchStatement* StatementParser::ParseSwitchStatement(TokenStream& stream)
    {
        stream.Consume();
        auto lp = stream.Consume();
        assert(lp->Is<LParen>());
        auto cond = expressionParser.Parse(stream);
        auto rp = stream.Consume();
        assert(rp->Is<RParen>());
        vector<pair<Expression*, Statement*>> cases;
        auto lb = stream.Consume();
        assert(lb->Is<LBrace>());
        for (auto next = stream.Peek(); next->IsNot<RBrace>(); next = stream.Peek())
        {
            auto expr = expressionParser.Parse(stream);
            assert(expr);
            if (auto id = expr->As<IdentifierExpression>(); id && id->Identifier == "_")
                expr = nullptr;
            auto lambda = stream.Consume();
            assert(lambda->Is<Lambda>());
            auto stat = Parse(stream);
            assert(stat);
            cases.emplace_back(expr, stat);
        }
        auto rb = stream.Consume();
        assert(rb->Is<RBrace>());
        return Create<SwitchStatement>(cond, move(cases));
    }

    VariableDeclarationBase* StatementParser::ParseVariableDeclaration(TokenStream& stream)
    {
        auto token = stream.Consume();
        auto result = stream.Peek()->Is<LParen>() ? (VariableDeclarationBase*)ParseImplicitTupleVariableDeclaration(stream, token) : ParseImplicitVariableDeclaration(stream, token);
        if (result->IsNot<PropertyDeclaration>())
        {
            auto semi = stream.Consume();
            assert(semi->Is<Semi>());
        }
        return result;
    }

    ClassDeclaration* StatementParser::ParseClassDeclaration(TokenStream& stream)
    {
        stream.Consume();
        auto name = stream.Consume();
        assert(name->Is<Identifier>());
        vector<VariableDeclaration*> defaultConstructorParameters;
        if (auto lp = stream.Peek(); lp->Is<LParen>())
        {
            stream.Consume();
            while (true)
            {
                auto next = stream.Peek();
                assert((next->IsOr<Val, Var, Identifier>()));
                auto decl = ParseImplicitVariableDeclaration(stream, !next->Is<Identifier>() ? stream.Consume() : nullptr)->As<VariableDeclaration>();
                assert(decl);
                defaultConstructorParameters.emplace_back(decl);
                if (stream.Peek()->Is<Comma>())
                {
                    stream.Consume();
                    continue;
                }
                auto rp = stream.Consume();
                assert(rp->Is<RParen>());
                break;
            }
        }
        vector<ClassInheritanceStatement*> inheritances;
        if (stream.Peek()->Is<Colon>())
        {
            stream.Consume();
            auto [v, tks] = ParseFunctionWhile<Comma>(stream, [&]
            {
                auto type = typeParser.Parse(stream)->As<IdentifierType>();
                assert(type);
                if (stream.Peek()->Is<LParen>())
                {
                    stream.Consume();
                    auto [exprs, tks] = expressionParser.ParseWhile<Comma>(stream);
                    auto rp = stream.Consume();
                    assert(rp->Is<RParen>());
                    return Create<ClassInheritanceStatement>(type, move(exprs));
                }
                return Create<ClassInheritanceStatement>(type);
            });
            inheritances = move(v);
        }
        auto dc = !defaultConstructorParameters.empty() ? Create<ConstructorDeclaration>(move(defaultConstructorParameters), EmptyVector{}, EmptyVector{}, false, false) : nullptr;
        if (dc && stream.Peek()->Is<Semi>())
        {
            stream.Consume();
            return Create<ClassDeclaration>((string)name->Text(), dc);
        }
        auto lb = stream.Consume();
        assert(lb->Is<LBrace>());
        vector<Statement*> statements;
        while (!stream.Peek()->Is<RBrace>())
            statements.emplace_back(Parse(stream));
        auto rb = stream.Consume();
        assert(rb->Is<RBrace>());
        return Create<ClassDeclaration>((string)name->Text(), dc, move(inheritances), move(statements));
    }

    pair<vector<VariableDeclaration*>, bool> StatementParser::ParseParameters(TokenStream& stream)
    {
        vector<VariableDeclaration*> parameters;
        bool isVariadic = false;
        while (true)
        {
            auto next = stream.Peek();

            switch (auto kind = next->KindId())
            {
            case Comma::Id:
                stream.Consume();
                if (stream.Peek()->Is<Ellipsis>())
                {
                    isVariadic = true;
                    stream.Consume();
                    return { parameters, isVariadic };
                }
                break;
            case Ellipsis::Id:
                isVariadic = true;
                stream.Consume();
                [[fallthrough]];
            case RParen::Id:
            case RBracket::Id:
                return { parameters, isVariadic };
            }
            auto p = ParseImplicitVariableDeclaration(stream, nullptr)->As<VariableDeclaration>();
            parameters.emplace_back(p);
        }
    }

    FunctionDeclaration* StatementParser::ParseFunctionDeclaration(TokenStream& stream)
    {
        stream.Consume();
        auto rb = stream.Consume();
        assert(rb->Is<RBracket>());
        auto name = stream.Consume();
        assert(name->Is<Identifier>());
        auto lp = stream.Consume();
        assert(lp->Is<LParen>());
        auto [parameters, isVariadic] = ParseParameters(stream);
        auto rp = stream.Consume();
        assert(rp->Is<RParen>());
        BaseType* retType = nullptr;
        if (stream.Peek()->Is<Colon>())
        {
            stream.Consume();
            retType = typeParser.Parse(stream);
            assert(retType);
        }
        vector<Statement*> statements;
        Expression* lambdaReturn = nullptr;
        switch (stream.Peek()->KindId())
        {
        case LBrace::Id:
        {
            stream.Consume();
            while (!stream.Peek()->Is<RBrace>())
            {
                auto statement = Parse(stream);
                assert(statement != nullptr);
                statements.emplace_back(statement);
            }
            stream.Consume();
        }break;
        case Lambda::Id:
        {
            stream.Consume();
            lambdaReturn = expressionParser.Parse(stream);
            auto semi = stream.Consume();
            assert(semi->Is<Semi>());
        }break;
        case Semi::Id:
            stream.Consume();
        break;
        default:
            assert(false);
            throw UnexpectedTokenException(stream.Peek());
        }
        return Create<FunctionDeclaration>((string)name->Text(), move(parameters), retType, isVariadic, move(statements), lambdaReturn);
    }

    OperatorDeclaration* StatementParser::ParseOperatorDeclaration(TokenStream& stream)
    {
        stream.Consume();
        Token* op = nullptr;
        switch (auto kind = stream.Peek()->KindId())
        {
        case LParen::Id:
        case LBracket::Id:
            op = stream.Peek();
        break;

        case Plus::Id:
        case Minus::Id:
        case Star::Id:
        case Slash::Id:
        case Modulo::Id:

        case Assign::Id:
        case PlusAssign::Id:
        case MinusAssign::Id:
        case StarAssign::Id:
        case SlashAssign::Id:
        case ModuloAssign::Id:

        case Increment::Id:
        case Decrement::Id:

        case Or::Id:
        case And::Id:

        case Ampersand::Id:
        case Pipe::Id:
        case Tilde::Id:
        case Caret::Id:
        case Exclamation::Id:

        case AmpersandAssign::Id:
        case PipeAssign::Id:
        case CaretAssign::Id:

        case LeftShift::Id:
        case RightShift::Id:
        case LeftShiftAssign::Id:
        case RightShiftAssign::Id:

        case Equal::Id:
        case NotEqual::Id:
        case Less::Id:
        case LessEqual::Id:
        case Greater::Id:
        case GreaterEqual::Id:
        case Spaceship::Id:
            op = stream.Consume();
        break;
        default:
            throw UnexpectedTokenException(stream.Peek());
        }
        auto lp = stream.Consume();
        assert((lp->IsOr<LParen, LBracket>()));
        auto [parameters, isVariadic] = ParseParameters(stream);
        auto rp = stream.Consume();
        assert((lp->Is<LParen>() && rp->Is<RParen>() || lp->Is<LBracket>() && rp->Is<RBracket>()));
        BaseType* retType = nullptr;
        if (stream.Peek()->Is<Colon>())
        {
            stream.Consume();
            retType = typeParser.Parse(stream);
            assert(retType);
        }
        vector<Statement*> statements;
        bool isLambdaReturn = false;
        switch (stream.Consume()->KindId())
        {
        case LBrace::Id:
            while (!stream.Peek()->Is<RBrace>())
            {
                auto statement = Parse(stream);
                assert(statement != nullptr);
                statements.emplace_back(statement);
            }
            stream.Consume();
            break;
        case Lambda::Id:
        {
            statements.emplace_back(Create<ExpressionStatement>(expressionParser.Parse(stream)));
            auto semi = stream.Consume();
            assert(semi->Is<Semi>());
            isLambdaReturn = true;
        }break;
        case Semi::Id:
            break;
        default:
            assert(false);
            return nullptr;
        }
        return Create<OperatorDeclaration>(op, move(parameters), retType, isLambdaReturn, move(statements));
    }

    VariableDeclarationBase* StatementParser::ParseImplicitVariableDeclaration(TokenStream& stream, Token* declToken)
    {
        auto name = stream.Consume();
        auto typeMod = name->IsOr<In, Out, Ref>() ? name : nullptr;
        if (typeMod)
            name = stream.Consume();
        assert(name->Is<Identifier>());
        BaseType* type = nullptr;
        if (stream.Peek()->Is<Colon>())
        {
            stream.Consume();
            type = typeParser.Parse(stream);
            assert(type);
        }
        Expression* expr = nullptr;
        switch (stream.Peek()->KindId())
        {
        case Assign::Id:
        {
            stream.Consume();
            expr = expressionParser.Parse(stream);
            assert(expr);
        }break;
        case Lambda::Id:
        {
            stream.Consume();
            auto g = Parse(stream);
            assert(g);
            return Create<PropertyDeclaration>(declToken, (string)name->Text(), type, nullptr, g, nullptr);
        }break;
        case LBrace::Id:
        {
            stream.Consume();
            Statement* g = nullptr;
            Statement* s = nullptr;
            while (true)
            switch (stream.Peek()->KindId())
            {
            case Get::Id:
            {
                stream.Consume();
                if (stream.Peek()->Is<Lambda>())
                {
                    stream.Consume();
                    auto e = expressionParser.Parse(stream);
                    assert(e);
                    g = Create<ExpressionStatement>(e);
                    auto semi = stream.Consume();
                    assert(semi->Is<Semi>());
                }
                else g = Parse(stream);
            }break;
            case Set::Id:
            {
                stream.Consume();
                if (stream.Peek()->Is<Lambda>())
                {
                    stream.Consume();
                    auto e = expressionParser.Parse(stream);
                    assert(e);
                    s = Create<ExpressionStatement>(e);
                    auto semi = stream.Consume();
                    assert(semi->Is<Semi>());
                }
                else s = Parse(stream);
            }break;
            case RBrace::Id:
                stream.Consume();
                return Create<PropertyDeclaration>(declToken, (string)name->Text(), type, expr, g, s);
            default:
                assert(false);
                throw UnexpectedTokenException(stream.Peek());
            }
        }break;
        }
        assert(type || expr);
        return Create<VariableDeclaration>(typeMod, declToken, (string)name->Text(), type, expr);
    }

    TupleVariableDeclaration* StatementParser::ParseImplicitTupleVariableDeclaration(TokenStream& stream, Token* declToken)
    {
        auto lp = stream.Consume();
        assert(lp->Is<LParen>());
        vector<string> names;
        while (stream.Peek()->Is<Identifier>())
        {
            auto n = stream.Consume();
            names.emplace_back((string)n->Text());
            if (stream.Peek()->Is<RParen>())
                break;
            auto comma = stream.Consume();
            assert(comma->Is<Comma>());
        }
        auto rp = stream.Consume();
        assert(rp->Is<RParen>());
        TupleType* type = nullptr;
        if (auto colon = stream.Peek(); colon->Is<Colon>())
        {
            stream.Consume();
            type = typeParser.Parse(stream)->As<TupleType>();
            assert(type);
        }
        Expression* expr = nullptr;
        if (stream.Peek()->Is<Assign>())
        {
            auto assign = stream.Consume();
            assert(assign->Is<Assign>());
            expr = expressionParser.Parse(stream);
        }
        assert(type || expr);
        return Create<TupleVariableDeclaration>(declToken, move(names), type, expr);
    }

    AttributeDeclaration* StatementParser::ParseAttributeDeclaration(TokenStream& stream)
    {
        stream.Consume();
        auto lb = stream.Consume();
        assert(lb->Is<LBracket>());
        auto [exprs, _] = expressionParser.ParseWhile<Comma>(stream);
        auto rb = stream.Consume();
        assert(rb->Is<RBracket>());
        return Create<AttributeDeclaration>(move(exprs));
    }

    ConstructorDeclaration* StatementParser::ParseConstructor(TokenStream& stream)
    {
        auto isDestructor = stream.Peek()->Is<Tilde>();
        if (isDestructor)
            stream.Consume();
        auto _this = stream.Consume();
        assert(_this->Is<This>());
        auto lp = stream.Consume();
        assert(lp->Is<LParen>());
        auto [params, tk] = ParseFunctionWhile<Comma>(stream, [&] { return stream.Peek()->IsNot<RParen>() ? ParseImplicitVariableDeclaration(stream, nullptr)->As<VariableDeclaration>() : nullptr; });
        auto rp = stream.Consume();
        assert(rp->Is<RParen>());
        vector<FieldInitializerStatement*> fields;
        if (!isDestructor && stream.Peek()->Is<Colon>())
        {
            stream.Consume();
            auto [fi, _] = ParseFunctionWhile<Comma>(stream, [&]
            {
                auto field = stream.Consume();
                assert(field->Is<Identifier>() || field->Is<This>());
                auto lp = stream.Consume();
                assert(lp->Is<LParen>());
                auto [exprs, _] = expressionParser.ParseWhile<Comma>(stream);
                auto rp = stream.Consume();
                assert(rp->Is<RParen>());
                return Create<FieldInitializerStatement>(field, move(exprs));
            });
            fields = move(fi);
        }
        bool isLambdaReturn = false;
        vector<Statement*> statements;
        switch (stream.Peek()->KindId())
        {
        case LBrace::Id:
        {
            stream.Consume();
            while (!stream.Peek()->Is<RBrace>())
            {
                auto statement = Parse(stream);
                assert(statement != nullptr);
                statements.emplace_back(statement);
            }
            stream.Consume();
        }break;
        case Lambda::Id:
        {
            stream.Consume();
            statements.emplace_back(Create<ExpressionStatement>(expressionParser.Parse(stream)));
            auto semi = stream.Consume();
            assert(semi->Is<Semi>());
            isLambdaReturn = true;
        }break;
        case Semi::Id:
            stream.Consume();
            break;
        default:
            assert(false);
            throw UnexpectedTokenException(stream.Peek());
        }
        return Create<ConstructorDeclaration>(move(params), move(fields), move(statements), isDestructor, isLambdaReturn);
    }

    Module Module::Parse(string name, TokenStream& stream)
    {
        Module mod{ move(name) };
        while (stream.Peek()->IsNot<Eof>())
            mod.Statements.emplace_back(mod.parser.Parse(stream));
        return mod;
    }

    Module::Module(string name) : Name(move(name))
    {
        
    }

}
