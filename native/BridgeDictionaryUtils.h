#pragma once

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <algorithm>
#include <string>

namespace bridge_dictionary
{
inline std::string to_std_string(const godot::String& value)
{
    return std::string(value.utf8().get_data());
}

inline std::string get_string_field(
    const godot::Dictionary& dictionary,
    const godot::String& key,
    const std::string& fallback = "")
{
    if (!dictionary.has(key))
    {
        return fallback;
    }

    return to_std_string(godot::String(dictionary[key]));
}

inline int get_int_field(const godot::Dictionary& dictionary, const godot::String& key, int fallback = 0)
{
    if (!dictionary.has(key))
    {
        return fallback;
    }

    return static_cast<int>(dictionary[key]);
}

inline int get_min_int_field(
    const godot::Dictionary& dictionary,
    const godot::String& key,
    int minimum,
    int fallback)
{
    return std::max(minimum, get_int_field(dictionary, key, fallback));
}

inline int get_clamped_int_field(
    const godot::Dictionary& dictionary,
    const godot::String& key,
    int minimum,
    int maximum,
    int fallback)
{
    return std::clamp(get_int_field(dictionary, key, fallback), minimum, maximum);
}
}
