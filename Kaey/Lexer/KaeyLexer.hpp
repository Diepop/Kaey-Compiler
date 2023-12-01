#pragma once
#include "Kaey/Utility.hpp"
#include "DeclareToken.hpp"

namespace Kaey::Lexer
{
    using std::string;
    using std::string_view;
    using std::vector;
    using std::unique_ptr;

    enum class SeekOrigin
    {
        Begin,
        Current
    };

    struct StringStream
    {
        using iterator = const char*;

        static constexpr size_t EXTRA_CAPACITY = 500;

        StringStream(string_view sv = {}) : position(0)
        {
            Insert(sv);
        }

        char Consume()
        {
            ++position;
            auto d = *CurrentIterator();
            DebugUpdate();
            return d;
        }

        char Peek(ptrdiff_t delta = 0) const
        {
            assert(delta >= 0);
            return position + delta < Count() ? CurrentIterator()[delta] : '\0';
        }

        char Seek(ptrdiff_t delta, SeekOrigin type = SeekOrigin::Current)
        {
            switch (type)
            {
            case SeekOrigin::Begin:
                position = 0;
            [[fallthrough]];
            case SeekOrigin::Current:
                position = std::min(position + delta, (ptrdiff_t)Count());
            break;
            default:
                throw std::invalid_argument("'type'");
            }
            DebugUpdate();
            return Peek();
        }

        void Insert(string_view s)
        {
            if (value.capacity() < s.length())
                value.reserve(value.capacity() + s.length() + EXTRA_CAPACITY);
            value.insert(value.begin() + position, s.begin(), s.end());
            DebugUpdate();
        }

        void Reset()
        {
            position = 0;
            DebugUpdate();
        }

        explicit operator bool() const { return Count() > position; }

        size_t Count() const { return value.length(); }

        ptrdiff_t CurrentPosition() const { return position; }
        iterator CurrentIterator() const { return begin() + CurrentPosition(); }

        iterator begin() const { return value.data(); }
        iterator end() const { return value.data() + value.length(); }

        static StringStream FromFile(std::filesystem::path path);

    private:
        string value;
        ptrdiff_t position;
#ifdef _DEBUG
        string_view current;
        char currentChar = '\0';
        char nextChar = '\0';
#endif
        void DebugUpdate()
        {
#ifdef _DEBUG
            current = { CurrentIterator(), end()};
            currentChar = Peek();
            nextChar = Peek(1);
#endif
        }
    };

    enum TokenChannel
    {
        UnfilteredChannel,
        HiddenChannel,
        FirstChannel,
        ChannelCount
    };

    struct Token : Variant<Token,
        struct WhiteSpace,
        struct LineComment, //comment
        struct BlockComment, /*comment*/

        struct If,
        struct Else,
        struct True,
        struct False,

        struct While,
        struct For,
        struct Break,
        struct Continue,
        struct Switch,
        struct Default,

        struct Var,
        struct Val,
        struct Get,
        struct Set,
        struct Class,
        struct This,
        struct Operator,
        struct Return,

        struct As,
        struct Is,

        struct Union,
        struct Using,
        struct AlignOf,
        struct TypeOf,
        struct SizeOf,
        struct Asm,
        struct Throw,
        struct Try,
        struct Catch,
        struct Namespace,
        struct New,
        struct Delete,
        struct Private,
        struct Public,
        struct Protected,
        struct Virtual,
        struct Override,
        struct Final,
        struct Yield,
        struct Await,
        struct Null,

        struct In,
        struct Out,
        struct Ref,

        struct FloatingPointLiteral,      // 3.14
        struct BinaryIntegerLiteral,      // 0b101010
        struct OctalIntegerLiteral,       // 01232
        struct HexadecimalIntegerLiteral, // 0x29A
        struct IntegerLiteral,            // 123
        struct RawStringLiteral,          // R"id(Hello World!)id"
        struct MultiLineStringLiteral,    // """\n...\n"""
        struct StringLiteral,             // "Hello World!"
        struct Identifier,                // asd

        struct DoubleQuestion,  // ??
        struct Question,        // ?

        struct Lambda,          // =>

        struct And,             // &&
        struct Or,              // ||

        struct Increment,       // ++
        struct Decrement,       // --

        struct LeftShiftAssign, // <<=
        struct RightShiftAssign,// >>=

        struct LeftShift,       // <<
        struct RightShift,      // >>

        struct Spaceship,       // <=>
        struct Equal,           // ==
        struct NotEqual,        // !=
        struct LessEqual,       // <=
        struct GreaterEqual,    // >=
        struct Less,            // <
        struct Greater,         // >

        struct Assign,          // =
        struct ColonAssign,     // :=

        struct PipeAssign,      // |=
        struct AmpersandAssign, // &=
        struct CaretAssign,     // ^=

        struct PlusAssign,      // +=
        struct MinusAssign,     // -=
        struct StarAssign,      // *=
        struct SlashAssign,     // /=
        struct ModuloAssign,    // %=

        struct Exclamation,     // !

        struct Pipe,            // |
        struct Ampersand,       // &
        struct Caret,           // ^
        struct Tilde,           // ~

        struct Plus,            // +
        struct Minus,           // -
        struct Star,            // *
        struct Slash,           // /
        struct Modulo,          // %

        struct LParen,          // (
        struct RParen,          // )
        struct LBracket,        // [
        struct RBracket,        // ]
        struct LBrace,          // {
        struct RBrace,          // }

        struct DoubleQuotes,    // "
        struct Quote,           // '
        struct Colon,           // :
        struct Semi,            // ;
        struct Comma,           // ,
        struct Ellipsis,        // ...
        
        struct Dot,             // .
        struct Arrow,           // ->

        struct HashTag,         // #

        struct Eof
        >
    {
        Token(size_t position, size_t line, size_t column, TokenChannel channel, string text, string name);
        virtual ~Token() = default;
        size_t Position() const { return position; }
        size_t Line() const { return line; }
        size_t Column() const { return column; }
        TokenChannel Channel() const { return channel; }
        string_view Name() const { return name; }
        string_view Text() const { return text; }
        int Length() const { return (int)Text().length(); }
        bool IsOperator() const { return const_cast<Token*>(this)->AsOperator() != nullptr; }
        virtual struct OperatorToken* AsOperator() { return nullptr; }
    private:
        size_t position;
        size_t line;
        size_t column;
        TokenChannel channel;
        string text;
        string name;
    };

    struct UnexpectedTokenException : std::runtime_error
    {
        UnexpectedTokenException(Token* token) : std::runtime_error("Unexpected token found: '{}'"_f(token->Text())) {  }
    };

    struct OperatorToken : Token
    {
        OperatorToken(size_t position, size_t line, size_t column, TokenChannel channel, string text, string name);
        int Priority() const;
        bool IsRightToLeft() const;
        bool IsBinaryOperator() const;
        friend std::strong_ordering operator<=>(const OperatorToken& lhs, const OperatorToken& rhs);
        OperatorToken* AsOperator() override { return this; }
    private:
        mutable std::optional<int> priority;
        mutable std::optional<bool> isRightToLeft;
        mutable std::optional<bool> isBinaryOperator;
    };

    struct RegexToken : Token
    {
        using MatchResults = std::match_results<StringStream::iterator>;
        RegexToken(size_t position, size_t line, size_t column, TokenChannel channel, int group, MatchResults result, string name);
        int Group() const { return group; }
        const MatchResults& Result() const { return result; }
        string_view Value() const { return value; }
    private:
        int group;
        MatchResults result;
        string value;
    };

    KAEY_REGEX_TOKEN(WhiteSpace, R"([\s]+)", 0, HiddenChannel);
    KAEY_REGEX_TOKEN(LineComment, R"(\/\/[^\n]*)", 0, HiddenChannel);
    KAEY_REGEX_TOKEN(BlockComment, R"(/\*[^*]*\*+(?:[^/*][^*]*\*+)*/)", 0, HiddenChannel);
    KAEY_REGEX_TOKEN(Identifier, R"([_a-zA-Z]\w*)", 0, FirstChannel);

    KAEY_REGEX_TOKEN(FloatingPointLiteral, R"(\d*\.\d+)", 0, FirstChannel);
    KAEY_REGEX_TOKEN(BinaryIntegerLiteral, R"((0b[01]+)(?:(i|u)(\d+)?)?)", 0, FirstChannel);
    KAEY_REGEX_TOKEN(OctalIntegerLiteral, R"((0[0-7]+)(?:(i|u)(\d+)?)?)", 0, FirstChannel);
    KAEY_REGEX_TOKEN(HexadecimalIntegerLiteral, R"((0x[\da-fA-F]+)(?:(i|u)(\d+)?)?)", 0, FirstChannel);
    KAEY_REGEX_TOKEN(IntegerLiteral, R"((\d+)(?:(i|u)(\d+)?)?)", 0, FirstChannel);

    KAEY_RESERVED_IDENTIFIER(If, "if", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Else, "else", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(True, "true", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(False, "false", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Var, "var", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Val, "val", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Get, "get", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Set, "set", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Class, "class", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(This, "this", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Operator, "operator", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Return, "return", FirstChannel);

    KAEY_RESERVED_IDENTIFIER(While, "while", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(For, "for", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Break, "break", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Continue, "continue", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Switch, "switch", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Default, "default", FirstChannel);

    KAEY_RESERVED_IDENTIFIER(As, "as", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Is, "is", FirstChannel);

    KAEY_RESERVED_IDENTIFIER(Union, "union", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Using, "using", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(AlignOf, "alignof", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(TypeOf, "typeof", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(SizeOf, "sizeof", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Asm, "asm", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Throw, "throw", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Try, "try", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Catch, "catch", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Namespace, "namespace", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(New, "new", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Delete, "delete", FirstChannel);

    KAEY_RESERVED_IDENTIFIER(Private, "private", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Public, "public", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Protected, "protected", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Virtual, "protected", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Override, "override", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Final, "final", FirstChannel);

    KAEY_RESERVED_IDENTIFIER(Yield, "yield", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Await, "await", FirstChannel);

    KAEY_RESERVED_IDENTIFIER(Null, "null", FirstChannel);

    KAEY_RESERVED_IDENTIFIER(In, "in", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Out, "out", FirstChannel);
    KAEY_RESERVED_IDENTIFIER(Ref, "ref", FirstChannel);

    KAEY_TOKEN(Lambda, "=>", FirstChannel);

    KAEY_TOKEN(DoubleQuestion, "??", FirstChannel);
    KAEY_TOKEN(Question, "?", FirstChannel);

    KAEY_OPERATOR_TOKEN(And, "&&", FirstChannel);
    KAEY_OPERATOR_TOKEN(Or, "||", FirstChannel);

    KAEY_OPERATOR_TOKEN(Increment, "++", FirstChannel);
    KAEY_OPERATOR_TOKEN(Decrement, "--", FirstChannel);
    KAEY_OPERATOR_TOKEN(LeftShift, "<<", FirstChannel);
    KAEY_OPERATOR_TOKEN(RightShift, ">>", FirstChannel);

    KAEY_OPERATOR_TOKEN(Assign, "=", FirstChannel);
    KAEY_OPERATOR_TOKEN(ColonAssign, ":=", FirstChannel);

    KAEY_OPERATOR_TOKEN(LeftShiftAssign, "<<=", FirstChannel);
    KAEY_OPERATOR_TOKEN(RightShiftAssign, ">>=", FirstChannel);

    KAEY_OPERATOR_TOKEN(AmpersandAssign, "&=", FirstChannel);
    KAEY_OPERATOR_TOKEN(PipeAssign, "|=", FirstChannel);
    KAEY_OPERATOR_TOKEN(CaretAssign, "^=", FirstChannel);

    KAEY_OPERATOR_TOKEN(PlusAssign, "+=", FirstChannel);
    KAEY_OPERATOR_TOKEN(MinusAssign, "-=", FirstChannel);
    KAEY_OPERATOR_TOKEN(StarAssign, "*=", FirstChannel);
    KAEY_OPERATOR_TOKEN(SlashAssign, "/=", FirstChannel);
    KAEY_OPERATOR_TOKEN(ModuloAssign, "%=", FirstChannel);

    KAEY_OPERATOR_TOKEN(Exclamation, "!", FirstChannel);

    KAEY_OPERATOR_TOKEN(Pipe, "|", FirstChannel);
    KAEY_OPERATOR_TOKEN(Ampersand, "&", FirstChannel);
    KAEY_OPERATOR_TOKEN(Caret, "^", FirstChannel);
    KAEY_OPERATOR_TOKEN(Tilde, "~", FirstChannel);

    KAEY_OPERATOR_TOKEN(Plus, "+", FirstChannel);
    KAEY_OPERATOR_TOKEN(Minus, "-", FirstChannel);
    KAEY_OPERATOR_TOKEN(Star, "*", FirstChannel);
    KAEY_OPERATOR_TOKEN(Slash, "/", FirstChannel);
    KAEY_OPERATOR_TOKEN(Modulo, "%", FirstChannel);

    KAEY_OPERATOR_TOKEN(Spaceship, "<=>", FirstChannel);
    KAEY_OPERATOR_TOKEN(Equal, "==", FirstChannel);
    KAEY_OPERATOR_TOKEN(NotEqual, "!=", FirstChannel);
    KAEY_OPERATOR_TOKEN(Less, "<", FirstChannel);
    KAEY_OPERATOR_TOKEN(LessEqual, "<=", FirstChannel);
    KAEY_OPERATOR_TOKEN(Greater, ">", FirstChannel);
    KAEY_OPERATOR_TOKEN(GreaterEqual, ">=", FirstChannel);

    KAEY_TOKEN(LParen, "(", FirstChannel);
    KAEY_TOKEN(RParen, ")", FirstChannel);
    KAEY_TOKEN(LBracket, "[", FirstChannel);
    KAEY_TOKEN(RBracket, "]", FirstChannel);
    KAEY_TOKEN(LBrace, "{", FirstChannel);
    KAEY_TOKEN(RBrace, "}", FirstChannel);

    KAEY_TOKEN(DoubleQuotes, "\"", FirstChannel);
    KAEY_TOKEN(Quote, "'", FirstChannel);
    KAEY_TOKEN(Colon, ":", FirstChannel);
    KAEY_TOKEN(Comma, ",", FirstChannel);
    KAEY_TOKEN(Semi, ";", FirstChannel);
    KAEY_TOKEN(Ellipsis, "...", FirstChannel);

    KAEY_TOKEN(Dot, ".", FirstChannel);
    KAEY_TOKEN(Arrow, "->", FirstChannel);

    KAEY_TOKEN(HashTag, "#", FirstChannel);

    KAEY_TOKEN(Eof, "\0", FirstChannel);

    KAEY_REGEX_TOKEN(MultiLineStringLiteral, R"(\"\"\"\n([\s\S]*)\"\"\")", 1, FirstChannel);
    KAEY_REGEX_TOKEN(RawStringLiteral, R"(R\"((?:\w+)?)\(([\s\S]*)\)\1\")", 2, FirstChannel);
    KAEY_REGEX_TOKEN(StringLiteral, R"(\"((?:[^\\\"\r\n]|\\.)*)\")", 0, FirstChannel);

    struct TokenStream
    {
        TokenStream(StringStream* ss = nullptr);

        TokenStream(StringStream& ss);

        void Seek(ptrdiff_t n, SeekOrigin origin);

        Token* Peek(ptrdiff_t n = 0);

        Token* Consume(ptrdiff_t n = 0);

        TokenChannel CurrentChannel() const;
        
        void SetCurrentChannel(TokenChannel channel);

        ptrdiff_t CurrentPosition() const;

        void SetCurrentPosition(ptrdiff_t pos);

    private:
        StringStream* ss;
        vector<unique_ptr<Token>(*)(StringStream*, ptrdiff_t, size_t, size_t)> regexes;
        vector<unique_ptr<Token>> tokens;
        TokenChannel currentChannel;
        Eof* eof;
#ifdef _DEBUG
        Token* currentToken = nullptr;
        Token* nextToken = nullptr;
        bool peeking = false;
#endif
    };

}
