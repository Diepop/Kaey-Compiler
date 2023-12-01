#pragma once

#define KAEY_DEFINE_OPERATOR(n1, n2) \
    inline const std::string n1 = [] \
    { \
        OperatorNames.emplace_back(n2); \
        return n2; \
    }()

namespace Kaey
{
    inline namespace Operators
    {
        inline std::vector<std::string_view> OperatorNames;

        inline const std::string CONSTRUCTOR = "$constructor";
        inline const std::string MOVE_OPERATOR = "$move";
        inline const std::string DESTRUCTOR = "$destructor";

        //Increment/Decrement
        KAEY_DEFINE_OPERATOR(ASSIGN_OPERATOR, "operator=");
        KAEY_DEFINE_OPERATOR(INCREMENT_OPERATOR, "operator++");
        KAEY_DEFINE_OPERATOR(DECREMENT_OPERATOR, "operator--");
        KAEY_DEFINE_OPERATOR(POST_INCREMENT_OPERATOR, "operator++post");
        KAEY_DEFINE_OPERATOR(POST_DECREMENT_OPERATOR, "operator--post");

        //Arithmetic
        KAEY_DEFINE_OPERATOR(PLUS_OPERATOR, "operator+");
        KAEY_DEFINE_OPERATOR(MINUS_OPERATOR, "operator-");
        KAEY_DEFINE_OPERATOR(STAR_OPERATOR, "operator*");
        KAEY_DEFINE_OPERATOR(SLASH_OPERATOR, "operator/");
        KAEY_DEFINE_OPERATOR(MODULO_OPERATOR, "operator%");

        KAEY_DEFINE_OPERATOR(PLUS_ASSIGN_OPERATOR, "operator+=");
        KAEY_DEFINE_OPERATOR(MINUS_ASSIGN_OPERATOR, "operator-=");
        KAEY_DEFINE_OPERATOR(STAR_ASSIGN_OPERATOR, "operator*=");
        KAEY_DEFINE_OPERATOR(SLASH_ASSIGN_OPERATOR, "operator/=");
        KAEY_DEFINE_OPERATOR(MODULO_ASSIGN_OPERATOR, "operator%=");

        //Binary
        KAEY_DEFINE_OPERATOR(TILDE_OPERATOR, "operator~");

        KAEY_DEFINE_OPERATOR(PIPE_OPERATOR, "operator|");
        KAEY_DEFINE_OPERATOR(AMPERSAND_OPERATOR, "operator&");
        KAEY_DEFINE_OPERATOR(CARET_OPERATOR, "operator^");
        KAEY_DEFINE_OPERATOR(LEFT_SHIFT_OPERATOR, "operator<<");
        KAEY_DEFINE_OPERATOR(RIGHT_SHIFT_OPERATOR, "operator>>");

        KAEY_DEFINE_OPERATOR(PIPE_ASSIGN_OPERATOR, "operator|=");
        KAEY_DEFINE_OPERATOR(AMPERSAND_ASSIGN_OPERATOR, "operator&=");
        KAEY_DEFINE_OPERATOR(CARET_ASSIGN_OPERATOR, "operator^=");
        KAEY_DEFINE_OPERATOR(LEFT_SHIFT_ASSIGN_OPERATOR, "operator<<=");
        KAEY_DEFINE_OPERATOR(RIGHT_SHIFT_ASSIGN_OPERATOR, "operator>>=");

        //Comparison
        KAEY_DEFINE_OPERATOR(STRICT_EQUAL_OPERATOR, "operator===");
        KAEY_DEFINE_OPERATOR(STRICT_NOT_EQUAL_OPERATOR, "operator!=="); 
        KAEY_DEFINE_OPERATOR(EQUAL_OPERATOR, "operator==");
        KAEY_DEFINE_OPERATOR(NOT_EQUAL_OPERATOR, "operator!=");
        KAEY_DEFINE_OPERATOR(LESS_OPERATOR, "operator<");
        KAEY_DEFINE_OPERATOR(GREATER_OPERATOR, "operator>");
        KAEY_DEFINE_OPERATOR(LESS_EQUAL_OPERATOR, "operator<=");
        KAEY_DEFINE_OPERATOR(GREATER_EQUAL_OPERATOR, "operator>=");
        KAEY_DEFINE_OPERATOR(SPACESHIP_OPERATOR, "operator<=>");

        //Logic
        KAEY_DEFINE_OPERATOR(NOT_OPERATOR, "operator!");
        KAEY_DEFINE_OPERATOR(AND_OPERATOR, "operator&&");
        KAEY_DEFINE_OPERATOR(OR_OPERATOR, "operator||");

        //Others
        KAEY_DEFINE_OPERATOR(SUBSCRIPT_OPERATOR, "operator[]");
        KAEY_DEFINE_OPERATOR(PARENTHESIS_OPERATOR, "operator()");
        KAEY_DEFINE_OPERATOR(ARROW_OPERATOR, "operator->");
        KAEY_DEFINE_OPERATOR(ELLIPSIS_OPERATOR, "operator...");

        KAEY_DEFINE_OPERATOR(ADDROF_OPERATOR, "$addrof");
        KAEY_DEFINE_OPERATOR(IS_OPERATOR, "$is");
        KAEY_DEFINE_OPERATOR(AS_OPERATOR, "$as");
        KAEY_DEFINE_OPERATOR(NULL_COALESCING_OPERATOR, "operator??");

    }
}

#undef KAEY_DEFINE_OPERATOR
