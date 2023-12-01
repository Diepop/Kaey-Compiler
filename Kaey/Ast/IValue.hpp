#pragma once
#include "IValueVisitor.hpp"

namespace Kaey::Ast
{
    struct ConstructOperator;
    struct PointerType;
    struct ReferenceType;
    struct IConstant;

    struct IValue : Accepter<IValueVisitor, IValue>
    {
        virtual ~IValue() = default;
        virtual IContext* Context() const = 0;
        virtual string_view Name() const { return {}; }

        virtual bool IsType() const { return false; }
        virtual bool IsExpression() const { return false; }
        virtual bool IsConstant() const { return false; }

        template<class T>
        T* As()
        {
            using std::is_same_v;
            if constexpr (is_same_v<T, IType>)
                return IsType() ? (IType*)this : nullptr;
            else if constexpr (is_same_v<T, IExpression>)
                return IsExpression() ? (IExpression*)this : nullptr;
            else if constexpr (is_same_v<T, IConstant>)
                return IsConstant() ? (IConstant*)this : nullptr;
            else return Accepter::As<T>();
        }

        template<class T>
        bool Is()
        {
            using std::is_same_v;
            if constexpr (is_same_v<T, IType>)
                return IsType();
            else if constexpr (is_same_v<T, IExpression>)
                return IsExpression();
            else if constexpr (is_same_v<T, IConstant>)
                return IsConstant();
            else return Accepter::Is<T>();
        }
        
    };

    

    struct IExpression : IValue
    {
        bool IsExpression() const final { return true; }
        virtual IType* Type() const = 0;
    };

    struct IConstant : IExpression
    {
        bool IsConstant() const final { return true; }
    };

}
