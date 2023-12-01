#pragma once
#include "Kaey/Utility.hpp"

namespace Kaey::Ast
{
    template<class T>
    struct IdentifierList : ArrayView<T*>
    {
        IdentifierList() : ArrayView<T*>(v) {  }

        T* Find(std::string_view name) const
        {
            auto it = map.find(name);
            return it != map.end() ? it->second : nullptr;
        }

        void Add(T* val)
        {
            auto name = val->Name();
            assert(!Find(name));
            map.emplace(name, val);
            v.emplace_back(val);
            static_cast<ArrayView<T*>&>(*this) = v;
        }

    private:
        std::unordered_map<std::string_view, T*> map;
        std::vector<T*> v;
    };
}

