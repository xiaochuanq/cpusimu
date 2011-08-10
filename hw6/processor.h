#ifndef PROCESSOR_H
#define PROCESSOR_H
#include <vector>
#include <deque>
#include <list>
#include <iostream>
#include <algorithm>
#include "stdint.h"
#include <cstdlib>
#include "instruction.h"
#include "gshare.h"

using namespace std;

class Processor {
protected:
	typedef deque<Instruction> ROB;
public:
	Processor(int nArchReg, int nPhyReg, uint32_t nRobDepth, uint32_t nWidth);
	virtual ~Processor();
	virtual void fetch(istream&);
	virtual void commit(ostream&);
	virtual void issue();
	virtual void advance();
	virtual void reset();
	virtual bool pipeline_empty() {
		return m_rob.empty();
	}
	uint32_t width() const {
		return m_nWidth;
	}
	uint32_t robCapacity() const {
		return m_nRobCapacity;
	}
	uint64_t currentCycle() const {
		return m_nCurrentCycle;
	}
	uint64_t totalMicro() const {
		return m_nTotalMicroops;
	}
	uint64_t totalMacro() const {
		return m_nTotalMacroops;
	}
protected:
	virtual bool isReady(const Instruction&) const = 0;
	Instruction* readInstruction(istream&);
	void renameInstruction(Instruction&);
	void unReady(const Instruction& instrn);
protected:
	uint32_t m_nRobCapacity;
	int m_nNumArchReg;
	int m_nNumPhyReg;
	uint32_t m_nWidth;

	gshare_predictor m_predictor;
	vector<int> m_maptable;
	deque<int> m_freelist;
	ROB m_rob;
	vector<int> m_scoreboard;

	uint64_t m_nCurrentCycle;
	uint64_t m_nTotalMacroops;
	uint64_t m_nTotalMicroops;
};

class Exp1Processor: public Processor {
public:
	Exp1Processor(int nArchReg, int nPhyReg, uint32_t nRobDepth, uint32_t nWidth) :
		Processor(nArchReg, nPhyReg, nRobDepth, nWidth) {
	}
	;
protected:
	bool isReady(const Instruction&) const;
};

class Exp2Processor: public Exp1Processor {
public:
	Exp2Processor(int nArchReg, int nPhyReg, uint32_t nRobDepth, uint32_t nWidth) :
		Exp1Processor(nArchReg, nPhyReg, nRobDepth, nWidth) {
	}
	;
protected:
	bool isReady(const Instruction&) const;
};

class Exp3Processor: public Exp1Processor {
public:
	Exp3Processor(int nArchReg, int nPhyReg, uint32_t nRobDepth, uint32_t nWidth) :
		Exp1Processor(nArchReg, nPhyReg, nRobDepth, nWidth) {
	}
	;
protected:
	bool isReady(const Instruction&) const;
};

class Exp4Processor: public Exp3Processor {
public:
	Exp4Processor(int nArchReg, int nPhyReg, uint32_t nRobDepth, uint32_t nWidth) :
		Exp3Processor(nArchReg, nPhyReg, nRobDepth, nWidth), m_nFetchReady(0) {
	}
	;
	void issue();
	void fetch(istream&);
private:
	int m_nFetchReady;
};

#endif
