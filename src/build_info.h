#pragma once
#include <string>
#include <sstream>
#include <cstdlib>
#include "version_autogen.h"

namespace build_info {

inline std::string compiler_runtime_id() {
#if defined(__clang__)
  return std::string("Clang ") + __clang_version__;
#elif defined(__GNUC__)
  return std::string("GCC ") + __VERSION__;
#elif defined(_MSC_VER)
  return "MSVC " + std::to_string(_MSC_VER);
#else
  return "UnknownCompiler";
#endif
}

inline std::string target_runtime_id() {
#if defined(_WIN32)
  constexpr const char* os = "Windows";
#elif defined(__APPLE__)
  constexpr const char* os = "macOS";
#elif defined(__linux__)
  constexpr const char* os = "Linux";
#else
  constexpr const char* os = "UnknownOS";
#endif

#if defined(__x86_64__) || defined(_M_X64)
  constexpr const char* arch = "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
  constexpr const char* arch = "aarch64";
#elif defined(__arm__) || defined(_M_ARM)
  constexpr const char* arch = "arm";
#elif defined(__riscv)
  constexpr const char* arch = "riscv";
#else
  constexpr const char* arch = "unknown-arch";
#endif

  std::string s = std::string(os) + "/" + arch;
  return s;
}

inline constexpr const char* cpp_standard() {
#if __cplusplus >= 202302L
    return "C++23";
#elif __cplusplus >= 202002L
    return "C++20";
#elif __cplusplus >= 201703L
    return "C++17";
#elif __cplusplus >= 201402L
    return "C++14";
#elif __cplusplus >= 201103L
    return "C++11";
#else
    return "pre-C++11";
#endif
}

// not used currently, defaults to 'Debug'
inline const char* build_type() {
#ifdef NDEBUG
  return "Release";
#else
  return "Debug";
#endif
}

// human-readable multi-line
inline std::string as_text() {
  std::ostringstream o;
  o << "Version:        " << version << "\n"
    << "Commit:         " << commit_short << " (" << commit_sha << ")\n"
    << "Branch:         " << branch << "\n"
    << "Tag:            " << tag << "\n"
    << "Dirty:          " << dirty << "\n"
    << "Repo:           " << repo_url << "\n"
    << "Commit date:    " << commit_date_utc << " (UTC)\n"
    << "Build date:     " << build_date_utc  << " (UTC)\n"
    << "Compiler:       " << compiler_runtime_id() << "\n"
    << "Compiler (cfg): " << compiler << "\n"
    << "C++ std:        " << cpp_standard() << "\n"
    << "Target:         " << target_runtime_id() << "\n"
    << "Build type:     " << build_type() << "\n"
    << "CXXFLAGS:       " << cxx_flags << "\n"
    << "LDFLAGS:        " << ld_flags  << "\n";
  return o.str();
}

} // namespace build_info
