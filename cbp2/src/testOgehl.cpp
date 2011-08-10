#include <iostream>
#include <cstdio>
#include <stdint.h>
#include "ogehl.hpp"

int failed = 0;
int total = 0;
#define expect(in, ex) _expect(in, ex, __LINE__)
void _expect(int32_t in, int32_t ex, int line) {
    total++;
    if (in != ex) {
        printf("testOgehl.cpp: Failed a test on line %d:\n", line);
        printf("               in = %d, expected %d\n", in, ex);
        failed++;
    }
}

#define test_lengths(l, e, n) _test_lengths(l, e, n, __LINE__)
void _test_lengths(const Lengths& l, int32_t expected[], int32_t n, int line) {
    _expect(l.size(), n, line);
    for (int32_t i = 0; i < n; i++) {
        _expect(l.at(i), expected[i], line);
    }
}

#define test_tables(t, es, el, n) _test_tables(t, es, el, n, __LINE__)
void _test_tables(const std::vector<Table *> *tables,
                  int32_t e_short[], int32_t e_long[],
                  int32_t n, int line) {
    _expect(tables->size(), n, line);
    for (int32_t i = 0; i < n; i++) {
        Table *t = tables->at(i);

        _expect(t->useLong(), 0, line);
        _expect(t->longHist(), e_long[i], line);
        _expect(t->shortHist(), e_short[i], line);
    }
}

//===========================================================================//
void populate(int32_t n, int32_t expected[],
              int32_t expec_short[], int32_t expec_long[],
              int32_t expec_long_ind[]) {
    for (int32_t i = 0; i < n; i++) {
        expec_long[i] = expected[expec_long_ind[i]];
        expec_short[i] = expected[i];
    }
}
//===========================================================================//

int main(int argc, char **argv) {
    //================================ Table ================================//
    {
        Table t(4, 0, 0, 2);
        t.debug(false);

        expect(t.numElements(), 16);

        expect(t.predict(0x1f, 0, 0), 0);
        t.update(0x1f, 0, 0, true);
        expect(t.predict(0x1f, 0, 0), 1);
        t.update(0x1f, 0, 0, true);
        t.update(0x1f, 0, 0, false);
        expect(t.predict(0x1f, 0, 0), 0);
    }

    {
        Table t(3, 4, 6, 4);

        expect(t.numElements(), 8);
    }

    // Compilcated index tests
    // {
    //     Table t(4, 3, 1, 1, 3);
    //     t.debug(true);

    //     expect(t.getComplicatedIndex(0xff, 1, 0), 0xe);
    //     expect(t.getComplicatedIndex(0xff, 0, 1), 0xe);
    //     t.update(0xff, 1, 1, 0);
    //     cout << "---" << endl;
    //     expect(t.getComplicatedIndex(0x1f, 7, 1), 0xc);
    //     expect(t.getComplicatedIndex(0x00, 1, 4), 0x3);
    //     t.update(0xff, 1, 1, 0);

    //     expect(t.getComplicatedIndex(0x00, 7, 1), 0xc);
    // }

    // {
    //     cout << endl << "4=" << binaryString(4, 4) << endl;
    //     cout << "4=" << binaryString(4, 8) << endl;
    //     cout << "15=" << binaryString(15, 4) << endl;
    //     cout << "14=" << binaryString(14, 4) << endl;
    // }


    //========================= Lengths and Tables ==========================//
    {
        // Lengths
        int32_t l1_sz = 2, num_lengths = 11;
        double alpha = 1.5;
        int32_t expected[] = {0, 2, 3, 5, 8, 12, 18, 27, 41, 62, 93};

        Lengths l(l1_sz, alpha, num_lengths);

        test_lengths(l, expected, num_lengths);

        // Tables
        int32_t idx_bits = 16, num_tables = 8, addtl_histl_vals = 3;
        int32_t ctr_bits = 4;
        std::vector<Table *> *t = makeTables(l, idx_bits, num_tables,
                                              addtl_histl_vals, ctr_bits);
        int32_t expec_long_ind[] = {0, 1, 8, 3, 9, 5, 10, 7};
        int32_t expec_long[num_tables], expec_short[num_tables];
        populate(num_tables, expected, expec_short, expec_long, expec_long_ind);
        test_tables(t, expec_short, expec_long, num_tables);
    }

    {
        int32_t l1_sz = 2, num_lengths = 10;
        double alpha = 1.8;
        int32_t expected[] = {0, 2, 4, 8, 15, 27, 49, 89, 161, 290};

        Lengths l(l1_sz, alpha, num_lengths);

        test_lengths(l, expected, num_lengths);
    }

    {
        // Lengths
        int32_t l1_sz = 2, num_lengths = 4;
        double alpha = 4;
        int32_t expected[] = {0, 2, 8, 32};

        Lengths l(l1_sz, alpha, num_lengths);

        test_lengths(l, expected, num_lengths);

        // Tables
        int32_t idx_bits = 16, num_tables = 3, addtl_histl_vals = 1;
        int32_t ctr_bits = 4;
        std::vector<Table *> *t = makeTables(l, idx_bits, num_tables,
                                              addtl_histl_vals, ctr_bits);
        int32_t expec_long_ind[] = {0, 3, 2};
        int32_t expec_long[num_tables], expec_short[num_tables];
        populate(num_tables, expected, expec_short, expec_long, expec_long_ind);
        test_tables(t, expec_short, expec_long, num_tables);
    }

    {
        int32_t l1_sz = 2, num_lengths = 5;
        double alpha = 3.1;
        int32_t expected[] = {0, 2, 7, 22, 69};

        Lengths l(l1_sz, alpha, num_lengths);

        test_lengths(l, expected, num_lengths);

        // Tables
        int32_t idx_bits = 10, num_tables = 4, addtl_histl_vals = 1;
        int32_t ctr_bits = 4;
        std::vector<Table *> *t = makeTables(l, idx_bits, num_tables,
                                              addtl_histl_vals, ctr_bits);
        int32_t expec_long_ind[] = {0, 1, 4, 3};
        int32_t expec_long[num_tables], expec_short[num_tables];
        populate(num_tables, expected, expec_short, expec_long, expec_long_ind);
        test_tables(t, expec_short, expec_long, num_tables);
    }

    {
        // Lengths
        int32_t l1_sz = 3, num_lengths = 6;
        double alpha = 3;
        int32_t expected[] = {0, 3, 9, 27, 81, 243};

        Lengths l(l1_sz, alpha, num_lengths);
        test_lengths(l, expected, num_lengths);

        // Tables
        int32_t idx_bits = 10, num_tables = 6, addtl_histl_vals = 0;
        int32_t ctr_bits = 4;
        std::vector<Table *> *t = makeTables(l, idx_bits, num_tables,
					     addtl_histl_vals, ctr_bits);
        int32_t expec_long_ind[] = {0, 1, 2, 3, 4, 5};
        int32_t expec_long[num_tables], expec_short[num_tables];
        populate(num_tables, expected, expec_short, expec_long, expec_long_ind);
        test_tables(t, expec_short, expec_long, num_tables);
    }

    //================================ Size =================================//
    {
        int32_t l1_sz = 2, num_lengths = 4;
        double alpha = 2;
	Lengths len(l1_sz, alpha, num_lengths);

	int32_t idx_bits = 10, num_tables = 4, addtl_histl_vals = 0;
        int32_t ctr_bits = 4;
	std::vector<Table *> *tables = makeTables(len, idx_bits, num_tables,
						  addtl_histl_vals, ctr_bits);


        int32_t theta = 4, a_ctr_bits = 9, thresh_bits = 7;
	ogehl_predictor og(theta, tables, idx_bits, thresh_bits, a_ctr_bits,
			   false, false);

	expect(og.bitSize(), 16430);

	// dynamic theta
	ogehl_predictor og2(theta, tables, idx_bits, thresh_bits, a_ctr_bits,
			    true, false);
	expect(og2.bitSize(), og.bitSize() + thresh_bits);

	// dynamic hist
	ogehl_predictor og3(theta, tables, idx_bits, thresh_bits, a_ctr_bits,
			    false, true);
	expect(og3.bitSize(), og.bitSize() + a_ctr_bits + tables->back()->numElements());

	// all
	ogehl_predictor og4(theta, tables, idx_bits, thresh_bits, a_ctr_bits,
			    true, true);
	expect(og4.bitSize(), og.bitSize() + a_ctr_bits + tables->back()->numElements() + thresh_bits);

    }

    if (failed != 0) {
        printf("\ntestCounter.cpp: Failed %d/%d tests\n", failed, total);
        return 1;
    } else {
        printf("testCounter.cpp: Passed all %i tests\n", total);
    }
}


