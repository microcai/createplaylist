
#pragma once

#include <cstdlib>
#include <utility>
#include <memory>

template <auto fn>
struct ptr_deleter_from
{
    template <typename T>
    constexpr void operator()(T* arg) const
    {
        fn(arg);
    }
};

template <typename T, auto fn>
using fn_unique_ptr = std::unique_ptr<T, ptr_deleter_from<fn>>;
