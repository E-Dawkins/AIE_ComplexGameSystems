#pragma once
#include <any>

class CustomAny : std::any
{
public:
    std::size_t _stored_size = 0;

public:
    template <typename T>
    CustomAny(T&&) : _stored_size{ sizeof(std::decay_t<T>) } { }

    // ...
};