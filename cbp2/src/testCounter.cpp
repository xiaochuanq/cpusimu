#include <iostream>
#include <cstdio>
#include "SaturatingCounter.hpp"

int failed = 0;
int total = 0;

#define expect(count, val) expec(count, val, __LINE__)
void expec(int32_t in, int32_t val, int line) {
    total++;
    if (in != val) {
        printf("testCounter.cpp: Failed a test on line %d:\n", line);
        printf("                 value = %d, expected %d\n", in, val);
        failed++;
    }
}

int main(int argc, char **argv) {
    {
        // Initialization test
        SaturatingCounter count(3);
        expect(count.size(), 3);
        expect(count.val(), 0);
        expect(count.isSaturated(), false);
    }

    {
        // Basic increment and decrement test
        SaturatingCounter count(2);
        expect(count.size(), 2);

        expect(count.val(), 0);
        count.inc();
        expect(count.val(), 1);
        expect(count.isSaturated(), true);

        count.dec();
        expect(count.val(), 0);

        count.dec();
        expect(count.val(), -1);
        expect(count.isSaturated(), false);

        count.dec();
        count.dec();
        expect(count.val(), -2);
        expect(count.isSaturated(), true);
        count.inc();
        expect(count.val(), -1);

        count.inc();
        count.inc();
        count.inc();
        expect(count.val(), 1);

        expect(count.size(), 2);
    }

    if (failed != 0) {
        printf("\ntestCounter.cpp: Failed %d/%d tests\n", failed, total);
        return 1;
    } else {
        printf("testCounter.cpp: Passed all tests\n");
    }
}
