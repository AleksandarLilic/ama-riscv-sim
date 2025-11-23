#pragma once

#include <filesystem>
#include <iostream>
#include <memory_resource>
#include <vector>
#include <cstdio>

inline std::string gen_out_dir(
    const std::string& test_elf_path,
    const std::string& out_dir_tag) {

    std::filesystem::path p(test_elf_path);
    std::string last_directory = p.parent_path().filename().string();
    std::string binary_name = p.stem().string();
    std::string path_out = last_directory + "_" + binary_name + "_out";
    if (!out_dir_tag.empty()) path_out += "_" + out_dir_tag;
    path_out = path_out + "/";
    bool exists = std::filesystem::exists(path_out);
    //if (exists) std::filesystem::remove_all(path_out);
    if (exists) return path_out;
    bool created = std::filesystem::create_directory(path_out);
    if (!created) {
        std::cerr << "Failed to create directory: " << path_out << std::endl;
        throw std::runtime_error("Failed to create directory");
    }
    return path_out;
}

// inherit from the parent, override to print (de)alloc and call parent's method
struct logging_resource : std::pmr::memory_resource {
    void* do_allocate(std::size_t bytes, std::size_t align) override {
        std::fprintf(stdout, "alloc %zu bytes (align %zu)\n", bytes, align);
        return std::pmr::get_default_resource()->allocate(bytes, align);
    }
    void do_deallocate(void* p, std::size_t bytes, std::size_t align) override {
        std::fprintf(stdout, "dealloc %zu bytes (align %zu)\n", bytes, align);
        std::pmr::get_default_resource()->deallocate(p, bytes, align);
    }
    bool do_is_equal(const std::pmr::memory_resource& other)
    const noexcept override {
        return this == &other;
    }
};

// usage:
// logging_resource logres;
// std::pmr::vector<int> v{ &logres };
