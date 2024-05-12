#include <string>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <filesystem>
#include <gtest/gtest.h>

#define CHECK_PASS "0x0340 (mscratch): 0x1"

class sim_test : public ::testing::TestWithParam<std::string> {
    protected:
        bool check_test_result(const std::string &test_path) {
            std::string sim_bin = "../src/ama-riscv-sim";
            std::string log_name = gen_log_name(test_path) + "_dump.log";
            std::string sim_cmd = sim_bin + " " + test_path + " > " + log_name;
            int test_result = system(sim_cmd.c_str());
            EXPECT_EQ(test_result, 0) << "Failed to run: <" << test_path \
                << "> with error code: " << test_result;
            
            // open output log file and check result
            std::ifstream log_file(log_name);
            std::string line;
            bool test_passed = false;
            while (std::getline(log_file, line)) {
                if (line.find(CHECK_PASS) != std::string::npos) {
                    test_passed = true;
                    break;
                }
            }
            
            EXPECT_TRUE(test_passed) << "Test failed: <" << test_path << ">";
            return true;
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
