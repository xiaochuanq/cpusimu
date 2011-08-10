// predictor.h
// This file declares branch_update and branch_predictor classes.
#ifndef PREDICTOR_H
#define PREDICTOR_H

#include "branch.h"

//#include "instruction.h"
class branch_update {
	bool _direction_prediction;
	unsigned int _target_prediction;

public:
	bool direction_prediction ()const { return _direction_prediction; }
	void direction_prediction (bool b) { _direction_prediction = b; }

	bool target_prediction ()const { return _target_prediction; }
	void target_prediction (unsigned int t) { _target_prediction = t; }

	branch_update (void) :
		_direction_prediction(false), _target_prediction(0) {}
};

class Instruction;

class branch_predictor {
protected:
        bool _debug;
public:
	virtual branch_update *predict (const branch_info &) = 0;
	virtual branch_update *predict (const branch_info & a, const Instruction & b) {
          return predict(a);
        }

	virtual void update (branch_update *, bool, unsigned int) {}
        virtual void update (branch_update * u, bool t, unsigned int a, const Instruction & b) {
            update(u, t, a);
        }

        virtual void reset(){};

	virtual ~branch_predictor (void) {}
	virtual int size() { return  -1;} // size in bits
        void setDebug(bool d) {
          _debug = d;
        }
	virtual void printConfig(){};

       int32_t byteSize() {
            return -1;
        }

};

#endif
