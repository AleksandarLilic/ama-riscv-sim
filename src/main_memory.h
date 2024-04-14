#pragma once

#include "defines.h"
#include "dev.h"

class main_memory : public dev {
    private:
        void burn(std::string test_bin);
    public:
        main_memory() = delete;
        main_memory(size_t size, std::string test_bin);
};
