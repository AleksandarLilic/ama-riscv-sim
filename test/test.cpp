#include <string>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <filesystem>
#include <stdexcept>
#include <gtest/gtest.h>

#include "../src/defines.h"

#define CHECK_PASS "0x051e tohost       : 0x00000001"
#define SIM_BIN "../../src/build_gtest/ama-riscv-sim " // runs from test subdir

struct cmd_setup {
    std::string log_name;
    std::string sim_cmd;
};

class sim_test : public ::testing::TestWithParam<std::string> {
    private:
        cmd_setup setup(const std::string &test_path) {
            cmd_setup cs;
            cs.log_name = gen_log_name(test_path) + "_dump.log";
            cs.sim_cmd = SIM_BIN + test_path + " > " + cs.log_name + " 2>&1";
            return cs;
        }

        bool find_str(const std::string &log_name, const std::string &str) {
            std::ifstream log_file(log_name);
            std::string line;
            while (std::getline(log_file, line))
                if (line.find(str) != std::string::npos)
                    return true;
            return false;
        }

    protected:
        bool check_test_result(const std::string &test_path) {
            cmd_setup cs = setup(test_path);
            int test_result = system(cs.sim_cmd.c_str());
            EXPECT_EQ(test_result, 0) << "Failed to run: <" << test_path \
                << "> with error code: " << test_result;

            // open output log file and check result
            bool test_passed = find_str(cs.log_name, CHECK_PASS);
            EXPECT_TRUE(test_passed) << "Test failed: <" << test_path << ">";
            return true;
        }

        bool check_error(const std::string &test_path,
                         const std::string &error_msg) {
            cmd_setup cs = setup(test_path);
            int test_result = system(cs.sim_cmd.c_str());
            EXPECT_NE(test_result, 0) << "Expected error code: " << error_msg;
            return find_str(cs.log_name, error_msg);
        }

    public:
        static std::string gen_log_name(const std::string& path) {
            std::filesystem::path p(path);
            std::string last_dir = p.parent_path().filename().string();
            std::string elf_name = p.stem().string();
            std::replace(last_dir.begin(), last_dir.end(), '-', '_');
            std::string formatted = last_dir + "_" + elf_name;
            return formatted;
        }
};

std::vector<std::string> read_testlist(const std::string& filename) {
    std::vector<std::string> paths;
    std::ifstream file(filename);
    std::string line;
    while (getline(file, line)) {
        if (!line.empty()) {
            paths.push_back(line);
        }
    }
    return paths;
}

struct test_name {
    std::string operator()
    (const ::testing::TestParamInfo<std::string>& info) const {
        return sim_test::gen_log_name(info.param);
    }
};

TEST_P(sim_test, expected_pass) {
    ASSERT_TRUE(check_test_result(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(
    Default,
    sim_test,
    // FIXME: take this from cli
    ::testing::ValuesIn(read_testlist("../gtest_testlist.txt")),
    test_name()
);

TEST_F(sim_test, missing_arguments) {
    ASSERT_TRUE(check_error("", "Option 'path' has no value"));
}

TEST_F(sim_test, elf_file_not_found) {
    ASSERT_TRUE(check_error("not_found", "Failed to load ELF file."));
}

/* FIXME: need to generate oversized elf file
TEST_F(sim_test, bin_file_oversized) {
    // generate dummy bin file larger than MEM_SIZE
    std::ofstream test_bin("oversized.bin", std::ios::binary);
    for (int i = 0; i <= MEM_SIZE+2; i++)
        test_bin << "0";
    test_bin.close();
    ASSERT_TRUE(
        check_error("oversized.bin", "File size is greater than memory size.")
    );
}
*/

/* FIXME: needs to be handled with traps now
#define UI_MSG(x) \
    "Unsupported instruction: <" x "> at 1000c: " + gen_hex_str(arr[3])

uint32_t arr[4] = {
    0x00004117, //auipc  x2,0x4
    0x00010113, //addi   x2,x2,0
    0x00001197  //auipc  x3,0x1
};

void write_bin(const std::string &filename) {
    std::ofstream test_bin(filename, std::ios::binary);
    test_bin.write(reinterpret_cast<char*>(arr), sizeof(arr));
    test_bin.close();
}

std::string gen_hex_str(uint32_t val) {
    char hex_str[9];
    std::sprintf(hex_str, "%08x", val);
    return std::string(hex_str);
}

// TODO: can these be parametrized with the existing class?
TEST_F(sim_test, unsupported_opcode) {
    arr[3] = 0x0000007f;
    std::string filename = "unsupported_opcode.bin";
    write_bin(filename);
    ASSERT_TRUE(check_error(filename, UI_MSG("opcode")));
}

TEST_F(sim_test, unsupported_al_reg) {
    arr[3] = 0x40003033;
    std::string filename = "unsupported_al_reg.bin";
    write_bin(filename);
    ASSERT_TRUE(check_error(filename, UI_MSG("al_reg_rv32i")));
}

TEST_F(sim_test, unsupported_al_imm) {
    arr[3] = 0x40001013;
    std::string filename = "unsupported_al_imm.bin";
    write_bin(filename);
    ASSERT_TRUE(check_error(filename, UI_MSG("al_imm")));
}

TEST_F(sim_test, unsupported_load) {
    arr[3] = 0x00003003;
    std::string filename = "unsupported_load.bin";
    write_bin(filename);
    ASSERT_TRUE(check_error(filename, UI_MSG("load")));
}

TEST_F(sim_test, unsupported_store) {
    arr[3] = 0x00007023;
    std::string filename = "unsupported_store.bin";
    write_bin(filename);
    ASSERT_TRUE(check_error(filename, UI_MSG("store")));
}

TEST_F(sim_test, unsupported_branch) {
    arr[3] = 0x00002063;
    std::string filename = "unsupported_branch.bin";
    write_bin(filename);
    ASSERT_TRUE(check_error(filename, UI_MSG("branch")));
}

TEST_F(sim_test, unsupported_jalr) {
    arr[3] = 0x00002067;
    std::string filename = "unsupported_jalr.bin";
    write_bin(filename);
    ASSERT_TRUE(check_error(filename, UI_MSG("jalr")));
}

TEST_F(sim_test, unsupported_sys) {
    arr[3] = 0x34004073;
    std::string filename = "unsupported_sys.bin";
    write_bin(filename);
    ASSERT_TRUE(check_error(filename, UI_MSG("sys")));
}

TEST_F(sim_test, unsupported_csr) {
    arr[3] = 0xfff05073;
    std::string filename = "unsupported_csr.bin";
    write_bin(filename);
    ASSERT_TRUE(check_error(filename, "Unsupported CSR. Address: 0xfff"));
}

TEST_F(sim_test, unsupported_misc_mem) {
    arr[3] = 0x0000200f;
    std::string filename = "unsupported_misc_mem.bin";
    write_bin(filename);
    ASSERT_TRUE(check_error(filename, UI_MSG("misc_mem")));
}
*/
