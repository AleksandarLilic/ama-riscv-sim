#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

namespace str_utils {
    inline bool contains(const std::string& s, const std::string& substr) {
        std::size_t pos = s.find(substr);
        return (pos != std::string::npos);
    }

    inline std::vector<std::string> split(const std::string& s, char delim) {
        std::vector<std::string> result;
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delim)) result.push_back(item);
        return result;
    }

    inline std::string join(
        const std::vector<std::string>& parts, const std::string& sep)
    {
        std::string result;
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i > 0) result += sep;
            result += parts[i];
        }
        return result;
    }

    inline std::string trim(std::string s) {
        auto not_space = [](unsigned char c) {
            return !std::isspace(c);
        };

        s.erase(
            s.begin(),
            std::find_if(s.begin(), s.end(), not_space)
        );
        s.erase(
            std::find_if(s.rbegin(), s.rend(), not_space).base(),
            s.end()
        );

        return s;
    }

    inline std::string replace_first(
        const std::string& s, const std::string& sub, const std::string& repl)
    {
        if (sub.empty() || repl.empty()) return s;
        std::string result = s;
        std::size_t pos = result.find(sub);
        if (pos != std::string::npos) {
            result.replace(pos, sub.length(), repl);
        }
        return result;
    }

    inline std::string replace_all(
        const std::string& s, const std::string& sub, const std::string& repl)
    {
        if (sub.empty() || repl.empty()) return s;
        std::string result = s;
        std::size_t pos = 0;
        while ((pos = result.find(sub, pos)) != std::string::npos) {
            result.replace(pos, sub.length(), repl);
            pos += repl.length();
        }
        return result;
    }

    inline std::string to_upper(std::string s) {
        std::transform(
            s.begin(),
            s.end(),
            s.begin(),
            [](unsigned char c) {
                return std::toupper(c);
            }
        );
        return s;
    }

    inline std::string to_lower(std::string s) {
        std::transform(
            s.begin(),
            s.end(),
            s.begin(),
            [](unsigned char c) {
                return std::tolower(c);
            }
        );
        return s;
    }

}
