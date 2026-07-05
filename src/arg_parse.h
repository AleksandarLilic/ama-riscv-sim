#pragma once

#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// ordered map of string -> T:
// preserves declaration order so help text lists options as they were written
template <typename T>
using ordered_map = std::vector<std::pair<std::string, T>>;

template <typename T>
typename ordered_map<T>::const_iterator find_arg(
    const ordered_map<T>& map, const std::string& arg)
{
    for (auto it = map.begin(); it != map.end(); it++) {
        if (it->first == arg) return it;
    }
    return map.end();
}

// if an entry's value already appeared earlier under a different name, that
// earlier name is treated as canonical and this one is annotated as its alias
template <typename T>
std::string gen_help_list(const ordered_map<T>& map) {
    std::ostringstream oss;
    for (auto it = map.begin(); it != map.end(); it++) {
        if (oss.tellp() > 0) oss << ", "; // comma and space after first entry
        oss << it->first;
        for (auto prev = map.begin(); prev != it; prev++) {
            if (prev->second == it->second) {
                oss << " (alias of '" << prev->first << "')";
                break;
            }
        }
    }
    return oss.str();
}

// handle passed args
template <typename T>
T resolve_arg(
    const std::string& name,
    const std::string& arg,
    const ordered_map<T>& map)
{
    // check if given arg is supported
    auto it = find_arg(map, arg);
    if (it == map.end()) {
        std::cout << "Invalid value for " << name << ": " << arg
                  << ". Valid options: " << gen_help_list(map) << std::endl;
        throw std::invalid_argument("");
    }
    return it->second;
}

// list variant: resolve each token, and de-duplicate
template <typename T>
std::vector<T> resolve_arg_list(
    const std::string& name,
    const std::vector<std::string>& args,
    const ordered_map<T>& map)
{
    std::set<T> seen;
    // check if all args are supprted
    for (const auto& arg : args) {
        auto it = find_arg(map, arg);
        if (it == map.end()) {
            std::cout << "Invalid value for " << name << ": " << arg
                      << ". Valid options: " << gen_help_list(map) << std::endl;
            throw std::invalid_argument("");
        }
        seen.insert(it->second);
    }
    return {seen.begin(), seen.end()};
}
