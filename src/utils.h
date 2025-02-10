#include <filesystem>
#include <iostream>

std::string gen_out_dir(
    const std::string& test_elf_path,
    const std::string& out_dir_tag) {

    std::filesystem::path p(test_elf_path);
    std::string last_directory = p.parent_path().filename().string();
    std::string binary_name = p.stem().string();
    std::string path_out = "out_" + last_directory + "_" + binary_name;
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
