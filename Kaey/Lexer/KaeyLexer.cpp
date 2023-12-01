#include "KaeyLexer.hpp"

namespace Kaey::Lexer
{

    StringStream StringStream::FromFile(std::filesystem::path path)
    {
        std::ifstream file;
        file.open(path);
        std::stringstream ss;
        ss << file.rdbuf();
        return StringStream(ss.str());
    }

    Token::Token(size_t position, size_t line, size_t column, TokenChannel channel, string text, string name) :
        position(position), line(line), column(column),
        channel(channel), text(move(text)), name(move(name))
    {

    }

    OperatorToken::OperatorToken(size_t position, size_t line, size_t column, TokenChannel channel, string text, string name) :
        Token(position, line, column, channel, move(text), move(name))
    {

    }

    int OperatorToken::Priority() const
    {
        return priority.has_value() ?
            *priority :
            priority.emplace([id = KindId()]
            {
                switch (id)
                {
                case Question::Id:
                case DoubleQuestion::Id:

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
                    return 16;

                case Or::Id:
                    return 15;
                case And::Id:
                    return 14;

                case Equal::Id:
                case NotEqual::Id:
                    return 13;

                case Less::Id:
                case LessEqual::Id:
                case Greater::Id:
                case GreaterEqual::Id:
                    return 12;

                case Spaceship::Id:
                    return 11;

                case Plus::Id:
                case Minus::Id:
                    return 10;

                case Star::Id:
                case Slash::Id:
                case Modulo::Id:
                    return 9;

                case LeftShift::Id:
                case RightShift::Id:
                    return 8;

                case Pipe::Id:
                    return 7;
                case Caret::Id:
                    return 6;
                case Ampersand::Id:
                    return 5;

                default:
                    assert(false);
                    throw std::runtime_error("Invalid Operator Token!");
                }
            }());
    }

    bool OperatorToken::IsRightToLeft() const
    {
        return isRightToLeft.has_value() ?
            *isRightToLeft :
            isRightToLeft.emplace(this->IsOr<
                Question,

                Assign,

                LeftShiftAssign,
                RightShiftAssign,

                AmpersandAssign,
                PipeAssign,
                CaretAssign,

                PlusAssign,
                MinusAssign,
                StarAssign,
                SlashAssign,
                ModuloAssign
            >());
    }

    bool OperatorToken::IsBinaryOperator() const
    {
        return isBinaryOperator.has_value() ?
            *isBinaryOperator :
            isBinaryOperator.emplace(this->IsOr<
                DoubleQuestion,

                Plus,
                Minus,
                Star,
                Slash,
                Modulo,

                Pipe,
                Ampersand,
                Caret,

                And,
                Or,

                LeftShift,
                RightShift,

                Assign,

                LeftShiftAssign,
                RightShiftAssign,

                AmpersandAssign,
                PipeAssign,
                CaretAssign,

                PlusAssign,
                MinusAssign,
                StarAssign,
                SlashAssign,
                ModuloAssign,

                Equal,
                NotEqual,

                Less,
                LessEqual,
                Greater,
                GreaterEqual
            >());
    }

    std::strong_ordering operator<=>(const OperatorToken& lhs, const OperatorToken& rhs)
    {
        return lhs.Priority() <=> rhs.Priority();
    }

    RegexToken::RegexToken(size_t position, size_t line, size_t column, TokenChannel channel, int group, MatchResults result, string name) :
        Token(position, line, column, channel, result.str(), move(name)),
        group(group), result(move(result)), value(Result()[group].str())
    {

    }

    TokenStream::TokenStream(StringStream* ss) : ss(ss), currentChannel(FirstChannel), eof(nullptr)
    {
        auto addTokens = [&]<class... Tokens>(std::tuple<Tokens...>*) { (regexes.emplace_back(&Tokens::Create), ...); };
        addTokens((Token::Types*)nullptr);
    }

    TokenStream::TokenStream(StringStream& ss) : TokenStream(&ss)
    {

    }

    void TokenStream::Seek(ptrdiff_t n, SeekOrigin origin)
    {
        switch (origin)
        {
        case SeekOrigin::Begin:
            ss->Reset();
            if (!n) break;
            [[fallthrough]];
        case SeekOrigin::Current:
            if (n > 0)
                Consume(n - 1);
            else ss->Seek((size_t)n);
            break;
        default: throw std::invalid_argument("origin");
        }
    }

    Token* TokenStream::Peek(ptrdiff_t n)
    {
    #ifdef _DEBUG
        peeking = true;
    #endif
        assert(n >= 0);
        auto p = ss->CurrentPosition();
        auto result = Consume(n);
        ss->Seek(p, SeekOrigin::Begin);
    #ifdef _DEBUG
        peeking = false;
    #endif
        return result;
    }

    Token* TokenStream::Consume(ptrdiff_t n)
    {
        assert(n >= 0);
        auto consumeToken = [this]() -> Token*
        {
            auto position = ss->CurrentPosition();
            if (!tokens.empty() && position <= tokens.back()->Position())
            {
                auto it = find_if(tokens.rbegin(), tokens.rend(), [=](auto& tk) { return tk->Position() == position; });
                if (it != tokens.rend())
                {
                    ss->Seek((*it)->Length());
                    return (*it).get();
                }
            }
            if (position >= ss->Count())
                return eof ? eof : eof = tokens.emplace_back(std::make_unique<Eof>(position, 0, 0, ""))->As<Eof>();
            for (auto& fn : regexes) if (auto ptr = fn(ss, position, std::count(ss->begin(), ss->begin() + position, '\n') + 1, 0))
            {
                ss->Seek(ptr->Length());
                return tokens.emplace_back(move(ptr)).get();
            }
            throw std::runtime_error("Unexpected token! {}");
        };
        Token* tk;
        do tk = consumeToken();
        while (!tk->Is<Eof>() && (currentChannel != UnfilteredChannel && tk->Channel() != currentChannel || n--));
#ifdef _DEBUG
        if (!peeking)
        {
            currentToken = tk;
            nextToken = currentToken->IsNot<Eof>() ? Peek() : nullptr;
        }
#endif
        return tk;
    }

    TokenChannel TokenStream::CurrentChannel() const
    {
        return currentChannel;
    }

    void TokenStream::SetCurrentChannel(TokenChannel channel)
    {
        currentChannel = channel;
    }

    ptrdiff_t TokenStream::CurrentPosition() const
    {
        return ss->CurrentPosition();
    }

    void TokenStream::SetCurrentPosition(ptrdiff_t pos)
    {
        ss->Seek(pos, SeekOrigin::Begin);
    }

}
