#include "IValue.hpp"
#include "IContext.hpp"
#include "Type/ReferenceType.hpp"

namespace Kaey::Ast
{
    PointerType* IType::PointerTo()
    {
        return Context()->GetPointerType(this);
    }

    ReferenceType* IType::ReferenceTo()
    {
        return Context()->GetReferenceType(this);
    }

    ReferenceType* IType::AddReference()
    {
        auto r = As<ReferenceType>();
        return r ? r : ReferenceTo();
    }

    IType* IType::Decay()
    {
        return this;
    }

    IType* IType::RemoveReference()
    {
        return this;
    }

    bool IType::IsConvertibleTo(IType* type)
    {
        if (type == this)
            return true;
        //auto decls = type->Constructor()->Declarations();
        //for (auto d : decls) if (auto pt = d->Type()->ParameterTypes(); pt.size() == 1 && (pt[0] == this || pt[0] == Decay()))
        //    return true;
        return false;
    }

}
