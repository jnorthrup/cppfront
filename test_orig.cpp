#include "include/cpp2util.h"
#include <vector>
#include <string>

void test() {
    std::vector<int> v = {1, 2, 3};
    auto result = CPP2_UFCS(size, v);
}
