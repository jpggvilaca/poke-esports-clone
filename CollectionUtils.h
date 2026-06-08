#pragma once

#include <algorithm>

template <typename Container, typename Value>
bool ContainsValue(const Container& values, const Value& value)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}
