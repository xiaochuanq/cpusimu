// bc2_predictor.h
// This file contains a saturating counter predictor class.
#ifndef BC2_PREDICTOR_H
#define BC2_PREDICTOR_H
#define TABLE_BITS 15
#include "predictor.h"
#include "gshare.h"
class bc2_predictor;

class bc2_update:public branch_update {
	friend class bc2_predictor;
public:
	unsigned int index;
};

class bc2_predictor : public branch_predictor {
public:


  bc2_predictor (int bits = TABLE_BITS):tablebits(bits) {
   	  int table_length = 1 << bits;
	  tab = new unsigned int[table_length];
	  memset (tab, 0, sizeof (unsigned int)* table_length);                       //Intinitialized as strongly not taken (0).
	  size_in_bits = 4 * table_length;
  }

	~bc2_predictor(){
	  delete tab;
	}

	void printConfig(){
	  if(_debug){
	    cout <<"Table Bits: "<<tablebits<<endl;
	  }
	}

  branch_update *predict (const branch_info & b) {
	  bi = b;
	  if(b.br_flags & BR_CONDITIONAL){
	          u.index = b.address & ( (1<<tablebits)-1);
                  u.direction_prediction (tab[u.index] >> 1);
		}	   
	  else
	  {
		  u.direction_prediction (true);
	  }
	  u.target_prediction (0);
    return &u;
  }

  void update (branch_update *u, bool taken, unsigned int target) {
    if (bi.br_flags & BR_CONDITIONAL) {
			unsigned int *c = &tab[((bc2_update*)u)->index];
			if (taken) {

				if (*c < 3) (*c)++;
			}
			else {
				if (*c > 0) (*c)--;
			}
		}
  }
protected:
	branch_info bi;
	bc2_update  u;
        int size_in_bits;
	int tablebits;
	unsigned int * tab;
};

#endif