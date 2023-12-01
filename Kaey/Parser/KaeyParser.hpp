#pragma once
#include "Kaey/Lexer/KaeyLexer.hpp"

namespace Kaey::Parser
{
    using std::string;
    using std::vector;
    using std::string_view;
    using std::ostream;
    using std::pair;
    using std::unique_ptr;
    using Lexer::Token;
    using Lexer::OperatorToken;
    using Lexer::RegexToken;

    struct ExpressionParser;
    struct TypeParser;
    struct BaseType;

    static constexpr auto& IDENT_SPACE = "  ";

    struct Expression : Variant<Expression,
        struct BooleanExpression,           // true, false
        struct IntegerExpression,           // 32, 0x29A, 0133, 0b1101001
        struct FloatingExpression,          // 3.14159265
        struct UnaryOperatorExpression,     // -13
        struct BinaryOperatorExpression,    // 5 * 4
        struct CallExpression,              // calc()
        struct IdentifierExpression,        // Identifier
        struct ParenthisedExpression,       // (...)
        struct StringLiteralExpression,     // "Hello World!"
        struct TupleExpression,             // ("Hello", 1, 3.14)
        struct SubscriptOperator,           // arr[1]
        struct TypeCastExpression,          // i as string, v as int
        struct TernaryExpression,           // cond ? 5 : 3
        struct MemberAccessExpression,      // this.field
        struct NewExpression,               // new Class(0, 0)
        struct NamedOperatorExpression,     // return 0, delete ptr, throw Exception("Error 404!")
        struct NamedTypeOperatorExpression  // sizeof i32, alignof Vector3
    >
    {
        virtual ~Expression() = default;
        virtual string ToString() const = 0;
    };

    struct BooleanExpression final : Expression::With<BooleanExpression>
    {
        bool Value;
        explicit BooleanExpression(bool value);
        string ToString() const override;
    };

    struct IntegerExpression final : Expression::With<IntegerExpression>
    {
        RegexToken* Token;
        uint64_t Value;
        int Bits;
        bool IsSigned;
        IntegerExpression(RegexToken* token);
        string ToString() const override;
    };

    struct FloatingExpression final : Expression::With<FloatingExpression>
    {
        RegexToken* Token;
        float Value;
        FloatingExpression(RegexToken* token);
        string ToString() const override;
    };

    struct UnaryOperatorExpression final : Expression::With<UnaryOperatorExpression>
    {
        Token* Operation;
        Expression* Operand;
        bool IsSuffix;
        UnaryOperatorExpression(Token* operation, Expression* operand, bool isPost = false);
        string ToString() const override;
    };

    struct BinaryOperatorExpression final : Expression::With<BinaryOperatorExpression>
    {
        Expression* Left;
        OperatorToken* Operation;
        Expression* Right;
        BinaryOperatorExpression(Expression* left, OperatorToken* operation, Expression* right);
        string ToString() const override;
    };

    struct CallExpression final : Expression::With<CallExpression>
    {
        Expression* Callee;
        vector<Expression*> Arguments;
        CallExpression(Expression* callee, vector<Expression*> arguments);
        string ToString() const override;
    };

    struct IdentifierExpression final : Expression::With<IdentifierExpression>
    {
        string Identifier;
        IdentifierExpression(string identifier);
        string ToString() const override;
    };

    struct ParenthisedExpression final : Expression::With<ParenthisedExpression>
    {
        Expression* UnderlyingExpression;
        explicit ParenthisedExpression(Expression* underlyingExpression);
        string ToString() const override;
    };

    struct TupleExpression final : Expression::With<TupleExpression>
    {
        vector<Expression*> Expressions;
        explicit TupleExpression(vector<Expression*> expressions);
        string ToString() const override;
    };

    struct StringLiteralExpression final : Expression::With<StringLiteralExpression>
    {
        RegexToken* Token;
        vector<char> Value;
        explicit StringLiteralExpression(RegexToken* token);
        string ToString() const override;
    };

    struct SubscriptOperator final : Expression::With<SubscriptOperator>
    {
        Expression* Callee;
        vector<Expression*> Arguments;
        SubscriptOperator(Expression* callee, vector<Expression*> arguments);
        string ToString() const override;
    };

    struct TypeCastExpression final : Expression::With<TypeCastExpression>
    {
        Expression* ConvertedExpression;
        Token* ConvertionType;
        BaseType* Type;
        TypeCastExpression(Expression* convertedExpression, Token* convertionType, BaseType* type);
        string ToString() const override;
    };

    struct TernaryExpression final : Expression::With<TernaryExpression>
    {
        Expression* Condition;
        Expression* TrueExpression;
        Expression* FalseExpression;
        TernaryExpression(Expression* condition, Expression* trueExpression, Expression* falseExpression);
        string ToString() const override;
    };

    struct MemberAccessExpression final : Expression::With<MemberAccessExpression>
    {
        Expression* Owner;
        IdentifierExpression* Member;
        MemberAccessExpression(Expression* owner, IdentifierExpression* memberIdentifier);
        string ToString() const override;
    };

    struct NewExpression final : Expression::With<NewExpression>
    {
        BaseType* Type;
        vector<Expression*> Dimension;
        vector<Expression*> Arguments;
        NewExpression(BaseType* type, vector<Expression*> dimension, vector<Expression*> arguments);
        string ToString() const override;
    };

    struct NamedOperatorExpression final : Expression::With<NamedOperatorExpression>
    {
        Token* Token;
        Expression* Argument;
        NamedOperatorExpression(Lexer::Token* token, Expression* argument);
        string ToString() const override;
    };

    struct NamedTypeOperatorExpression final : Expression::With<NamedTypeOperatorExpression>
    {
        Token* Token;
        BaseType* Type;
        NamedTypeOperatorExpression(Lexer::Token* token, BaseType* type);
        string ToString() const override;
    };

    struct BaseType : Variant<BaseType,
        struct IdentifierType,
        struct DecoratedType,
        struct TupleType,
        struct ArrayType,
        struct VariantType
    >
    {
        virtual ~BaseType() = default;
        virtual string ToString() const = 0;
    };

    struct IdentifierType final : BaseType::With<IdentifierType>
    {
        string Identifier;
        explicit IdentifierType(string identifier);
        string ToString() const override;
    };

    struct DecoratedType final : BaseType::With<DecoratedType>
    {
        BaseType* UnderlyingType;
        Token* Decorator;
        explicit DecoratedType(BaseType* underlyingType, Token* decorator);
        string ToString() const override;
    };

    struct TupleType final : BaseType::With<TupleType>
    {
        vector<BaseType*> Types;
        explicit TupleType(vector<BaseType*> types);
        string ToString() const override;
    };

    struct ArrayType final : BaseType::With<ArrayType>
    {
        BaseType* UnderlyingType;
        vector<Expression*> LengthExpressions;
        ArrayType(BaseType* underlyingType, vector<Expression*> lengthExpressions);
        string ToString() const override;
    };

    struct VariantType final : BaseType::With<VariantType>
    {
        vector<BaseType*> UnderlyingTypes;
        VariantType(vector<BaseType*> UnderlyingTypes);
        string ToString() const override;
    };

    template<class Result>
    struct Parser
    {
        using TokenStream = Lexer::TokenStream;

        virtual Result* Parse(TokenStream& stream) = 0;

        template<class... Tokens>
        pair<vector<Result*>, vector<Token*>> ParseWhile(TokenStream& stream, bool isOptional = false)
        {
            pair<vector<Result*>, vector<Token*>> result;
            auto& [exprs, tks] = result;
            while (true)
            {
                auto expr = Parse(stream);
                if (!expr && !isOptional)
                    break;
                exprs.emplace_back(expr);
                auto next = stream.Peek();
                if (!next->IsOr<Tokens...>())
                    break;
                tks.emplace_back(stream.Consume());
            }
            return result;
        }

        template<class... Tokens, class Fn>
        auto ParseFunctionWhile(TokenStream& stream, Fn fn, bool isOptional = false)
        {
            pair<vector<std::invoke_result_t<Fn>>, vector<Token*>> result;
            auto& [exprs, tks] = result;
            while (true)
            {
                auto expr = fn();
                if (!expr && !isOptional)
                    break;
                exprs.emplace_back(expr);
                auto next = stream.Peek();
                if (!next->IsOr<Tokens...>())
                    break;
                tks.emplace_back(stream.Consume());
            }
            return result;
        }

        bool Destroy(Result* ptr)
        {
            auto it = rn::find_if(ptrs, [=](auto& p) { return p.get() == ptr; });
            if (it == ptrs.end())
                return false;
            ptrs.erase(it);
            return true;
        }

    protected:
        template<class T, class... Args>
        T* Create(Args&&... args)
        {
            return (T*)ptrs.emplace_back(std::make_unique<T>(std::forward<Args>(args)...)).get();
        }

    private:
        vector<unique_ptr<Result>> ptrs;
    };

    struct TypeParser : Parser<BaseType>
    {
        explicit TypeParser(ExpressionParser* expressionParser);

        BaseType* Parse(TokenStream& stream) override;

    private:
        ExpressionParser* expressionParser;

        BaseType* ParseUnaryType(TokenStream& stream);

    };

    struct ExpressionParser : Parser<Expression>
    {
        explicit ExpressionParser(TypeParser* typeParser);

        Expression* Parse(TokenStream& stream) override;

        Expression* ParseUnaryExpression(TokenStream& stream, Token* unaryToken = nullptr);

    private:
        TypeParser* typeParser;

        Expression* ParseBinaryExpression(TokenStream& stream, Expression* lhs, const OperatorToken* lastOp = nullptr);
    };

    struct Statement : Variant<Statement,
        struct ExpressionStatement,
        struct BlockStatement,
        struct IfStatement,
        struct WhileStatement,
        struct ForStatement,
        struct VariableDeclaration,
        struct TupleVariableDeclaration,
        struct PropertyDeclaration,
        struct ClassDeclaration,
        struct FunctionDeclaration,
        struct OperatorDeclaration,
        struct ClassInheritanceStatement,
        struct AttributeDeclaration,
        struct ConstructorDeclaration,
        struct FieldInitializerStatement,
        struct SwitchStatement,
        struct EofStatement
    >
    {
        vector<AttributeDeclaration*> Attributes;
        explicit Statement(vector<AttributeDeclaration*> attributes = {});
        virtual ~Statement() = default;
        virtual void Print(ostream& os, string ident = {}) = 0;
    };

    struct ExpressionStatement final : Statement::With<ExpressionStatement>
    {
        Expression* UnderlyingExpression;
        explicit ExpressionStatement(Expression* underlyingExpression);
        void Print(ostream& os, string ident) override;
    };

    struct BlockStatement final : Statement::With<BlockStatement>
    {
        vector<Statement*> Statements;
        explicit BlockStatement(vector<Statement*> statements);
        void Print(ostream& os, string ident) override;
    };

    struct IfStatement final : Statement::With<IfStatement>
    {
        Expression* Condition;
        Statement* TrueStatement;
        Statement* FalseStatement;
        IfStatement(Expression* condition, Statement* trueStatement, Statement* falseStatement = nullptr);
        void Print(ostream& os, string ident) override;
    };

    struct WhileStatement final : Statement::With<WhileStatement>
    {
        Expression* Condition;
        Statement* LoopStatement;
        WhileStatement(Expression* condition, Statement* loopStatement);
        void Print(ostream& os, string ident) override;
    };

    struct ForStatement final : Statement::With<ForStatement>
    {
        Statement* StartStatement;
        Expression* Condition;
        Expression* Increment;
        Statement* LoopStatement;
        ForStatement(Statement* startStatement, Expression* condition, Expression* increment, Statement* loopStatement);
        void Print(ostream& os, string ident) override;
    };

    struct EofStatement final : Statement::With<EofStatement>
    {
        void Print(ostream& os, string ident) override;
    };

    struct VariableDeclarationBase : Statement
    {
        Token* DeclarationToken;
        Expression* InitializingExpression;
        VariableDeclarationBase(Token* declarationToken, Expression* initializingExpression, vector<AttributeDeclaration*> attributes = {});
    };

    struct VariableDeclaration final : Statement::With<VariableDeclaration, VariableDeclarationBase>
    {
        Token* TypeMod;
        string Name;
        BaseType* Type;
        VariableDeclaration(Token* typeMod, Token* declarationToken, string name, BaseType* type, Expression* initializingExpression);
        void Print(ostream& os, string ident) override;
    };

    struct TupleVariableDeclaration final : Statement::With<TupleVariableDeclaration, VariableDeclarationBase>
    {
        vector<string> Names;
        TupleType* Type;
        TupleVariableDeclaration(Token* declarationToken, vector<string> names, TupleType* type, Expression* initializingExpression);
        void Print(ostream& os, string ident) override;
    };

    struct PropertyDeclaration final : Statement::With<PropertyDeclaration, VariableDeclarationBase>
    {
        string Name;
        BaseType* Type;
        Statement* Getter;
        Statement* Setter;
        PropertyDeclaration(Token* declarationToken, string name, BaseType* type, Expression* initializingExpression, Statement* getter, Statement* setter);
        void Print(ostream& os, string ident) override;
    };

    struct ClassDeclaration final : Statement::With<ClassDeclaration>
    {
        string Name;
        ConstructorDeclaration* DefaultConstructor;
        vector<ClassInheritanceStatement*> Inheritances;
        vector<Statement*> Statements;
        ClassDeclaration(string name, ConstructorDeclaration* defaultConstructor, vector<ClassInheritanceStatement*> inheritances = {}, vector<Statement*> statements = {});
        void Print(ostream& os, string ident) override;
    };

    struct ClassInheritanceStatement final : Statement::With<ClassInheritanceStatement>
    {
        IdentifierType* Type;
        vector<Expression*> Arguments;
        ClassInheritanceStatement(IdentifierType* type, vector<Expression*> arguments = {});
        void Print(ostream& os, string ident) override;
    };

    struct FunctionDeclaration : Statement::With<FunctionDeclaration>
    {
        string Name;
        vector<VariableDeclaration*> Parameters;
        BaseType* ReturnType;
        bool IsVariadic;
        vector<Statement*> Statements;
        Expression* LambdaReturn;
        FunctionDeclaration(string name, vector<VariableDeclaration*> parameters, BaseType* returnType, bool isVariadic, vector<Statement*> statements, Expression* lambdaReturn);
        void Print(ostream& os, string ident) override;
    };

    struct OperatorDeclaration final : Statement::With<OperatorDeclaration>
    {
        Token* Operator;
        vector<VariableDeclaration*> Parameters;
        BaseType* ReturnType;
        bool IsLambdaReturn;
        vector<Statement*> Statements;
        OperatorDeclaration(Token* op, vector<VariableDeclaration*> parameters, BaseType* returnType, bool isLambdaReturn, vector<Statement*> statements);
        void Print(ostream& os, string ident) override;
    };

    struct ConstructorDeclaration final : Statement::With<ConstructorDeclaration>
    {
        vector<VariableDeclaration*> Parameters;
        vector<FieldInitializerStatement*> FieldInitializers;
        vector<Statement*> Statements;
        bool IsDestructor;
        bool IsLambdaReturn;
        ConstructorDeclaration(vector<VariableDeclaration*> parameters, vector<FieldInitializerStatement*> fieldInitializers, vector<Statement*> statements, bool isDestructor, bool isLambdaReturn);
        void Print(ostream& os, string ident) override;
    };

    struct FieldInitializerStatement final : Statement::With<FieldInitializerStatement>
    {
        Token* Field;
        vector<Expression*> Arguments;
        FieldInitializerStatement(Token* field, vector<Expression*> arguments);
        void Print(ostream& os, string ident) override;
    };

    struct AttributeDeclaration final : Statement::With<AttributeDeclaration>
    {
        vector<Expression*> Expressions;
        AttributeDeclaration(vector<Expression*> expressions);
        void Print(ostream& os, string ident) override;
    };

    struct SwitchStatement final : Statement::With<SwitchStatement>
    {
        Expression* Condition;
        vector<pair<Expression*, Statement*>> Cases;
        SwitchStatement(Expression* condition, vector<pair<Expression*, Statement*>> cases);
        void Print(ostream& os, string ident) override;
    };

    struct StatementParser final : Parser<Statement>
    {
        using TokenStream = Lexer::TokenStream;

        StatementParser();

        Statement* Parse(TokenStream& stream) override;

    private:
        ExpressionParser expressionParser;
        TypeParser typeParser;
        EofStatement eofStatement;

        IfStatement* ParseIf(TokenStream& stream);

        BlockStatement* ParseBlock(TokenStream& stream);

        WhileStatement* ParseWhileStatement(TokenStream& stream);

        ForStatement* ParseForStatement(TokenStream& stream);

        SwitchStatement* ParseSwitchStatement(TokenStream& stream);

        VariableDeclarationBase* ParseVariableDeclaration(TokenStream& stream);

        ClassDeclaration* ParseClassDeclaration(TokenStream& stream);

        pair<vector<VariableDeclaration*>, bool> ParseParameters(TokenStream& stream);

        FunctionDeclaration* ParseFunctionDeclaration(TokenStream& stream);

        OperatorDeclaration* ParseOperatorDeclaration(TokenStream& stream);

        VariableDeclarationBase* ParseImplicitVariableDeclaration(TokenStream& stream, Token* declToken);

        TupleVariableDeclaration* ParseImplicitTupleVariableDeclaration(TokenStream& stream, Token* declToken);

        AttributeDeclaration* ParseAttributeDeclaration(TokenStream& stream);

        ConstructorDeclaration* ParseConstructor(TokenStream& stream);

    };

    struct Module
    {
        string Name;
        vector<Statement*> Statements;
        static Module Parse(string name, Lexer::TokenStream& stream);
    private:
        StatementParser parser;
        Module(string name);
    };

}
