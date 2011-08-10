// gshare.h
// This file contains a sample gshare_predictor class.
#ifndef GSHARE_H
#define GSHARE_H
#include "predictor.h"

class gshare_update : public branch_update {
public:
	unsigned int index;
};

#define HISTORY_LENGTH	15
#define TABLE_BITS	15

class gshare_predictor : public branch_predictor {
public:
  gshare_predictor (int h = HISTORY_LENGTH, int bits = TABLE_BITS) :tablebits(bits), hlen(h), history(0) {
	  if( h> bits || h > 8*sizeof(unsigned long long int))
	    cerr << "Warning: history length exceeds table bits or the longest integer" << endl;
	  int table_length = 1 << bits;
	  tab = new unsigned char[table_length];
	  memset (tab, 0, sizeof (unsigned char)* table_length);
	  size_in_bits = hlen /*history bit*/ + 4/*four bits counter*/ * table_length;
	}

	~gshare_predictor(){
	  delete tab;
	}

	void printConfig(){
	  if(_debug){
	    cerr << "Table bits:    "<< tablebits << endl;
	    cerr << "History length:"<< hlen << endl;
   
	  }
	}
	branch_update *predict (const branch_info & b) {
		bi = b;
		if (b.br_flags & BR_CONDITIONAL) {
			u.index =
				  (history << (tablebits - hlen))
			  ^ (b.address & ((1<<tablebits)-1));
			u.direction_prediction (tab[u.index] >> 1);
		} else {
			u.direction_prediction (true);
		}
		u.target_prediction (0);
		return &u;
	}

	void update (branch_update *u, bool taken, unsigned int target) {
		if (bi.br_flags & BR_CONDITIONAL) {
			unsigned char *c = &tab[((gshare_update*)u)->index];
			if (taken) {
				if (*c < 3) (*c)++;
			} else {
				if (*c > 0) (*c)--;
			}
			history <<= 1;
			history |= taken;
			history &= (1<<hlen)-1;
		}
	}
	
	int size(){ return size_in_bits; }
 private:
	int tablebits;
	gshare_update u;
	int size_in_bits;
	branch_info bi;
	unsigned long long int history;
	int hlen;
	//unsigned char tab[1<<TABLE_BITS];
	unsigned char * tab;

};

#endif
