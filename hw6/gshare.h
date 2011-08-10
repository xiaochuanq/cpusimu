// gshare.h
// This file contains a sample gshare_predictor class.
// It is a simple 32,768-entry gshare with a history length of 15.
// Note that this predictor doesn't use the whole 32 kilobytes available
// for the CBP-2 contest; it is just an example.

#include <stdint.h>
#include <string.h>
#include "instruction.h"

#define HISTORY_LENGTH	15
#define TABLE_BITS	15

class gshare_predictor {
public:
	gshare_predictor(void) :
		history(0) {
		memset(tab, 0, sizeof(tab));
	}

	bool predict(uint64_t pc) {
		index = (history << (TABLE_BITS - HISTORY_LENGTH)) ^ (pc & ((1
				<< TABLE_BITS) - 1));
		return (tab[index] >> 1);
	}

	void reset() {
		history = 0;
		memset(tab, 0, sizeof(tab));
	}

	void update(bool taken, unsigned int targetPC) {
		unsigned char *pc = &tab[index];
		if (taken) {
			if (*pc < 3)
				(*pc)++;
		} else {
			if (*pc > 0)
				(*pc)--;
		}
		history <<= 1;
		history |= taken ? 1 : 0;
		history &= (1 << HISTORY_LENGTH) - 1;
	}
private:
	unsigned int history;
	unsigned char tab[1 << TABLE_BITS];
	unsigned int index;
};
