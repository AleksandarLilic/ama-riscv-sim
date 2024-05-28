#include <string>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <filesystem>
#include <stdexcept>
#include <gtest/gtest.h>

#include "../src/defines.h"

#define CHECK_PASS "0x0340 (mscratch): 0x1"
#define SIM_BIN "../src/ama-riscv-sim "

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

        bool check_exception(const std::string &test_path,
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
            std::string binary_name = p.stem().string();
            std::replace(last_dir.begin(), last_dir.end(), '-', '_');
            std::string formatted = last_dir + "_" + binary_name;
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
    ::testing::ValuesIn(read_testlist("gtest_testlist.txt")),
    test_name()
);

TEST_F(sim_test, missing_arguments) {
    ASSERT_TRUE(check_exception("", "Missing arguments."));
}

TEST_F(sim_test, bin_file_not_found) {
    ASSERT_TRUE(check_exception("not_found", "BIN failed to open."));
}

TEST_F(sim_test, bin_file_oversized) {
    // generate dummy bin file larger than MEM_SIZE
    std::ofstream test_bin("oversized.bin", std::ios::binary);
    for (int i = 0; i <= MEM_SIZE; i++)
        test_bin << "0";
    test_bin.close();
    ASSERT_TRUE(check_exception("oversized.bin",
                                "File size is greater than memory size.")
    );
}
