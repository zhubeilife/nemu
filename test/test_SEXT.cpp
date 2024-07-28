#include <stdint.h>
#include <iostream>
#include <bitset>

#define SEXT(x, len) ({ struct { int64_t n : len; } __x = { .n = x }; (uint64_t)__x.n; })

// test how SEXT works
int main()
{
    uint8_t x = 0b00000001;
    uint8_t y = 0b10000001;

    std::cout << "x = " << std::bitset<64>(SEXT(x, 8)) << std::endl;
    std::cout << "y = " << std::bitset<64>(SEXT(y, 8)) << std::endl;

    // x = 0000000000000000000000000000000000000000000000000000000000000001
    // y = 1111111111111111111111111111111111111111111111111111111110000001

    std::cout << "y >> 2 = " << std::bitset<64>((uint64_t)((int8_t)y >> 2)) << std::endl;
}