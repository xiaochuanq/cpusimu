#ifndef CIS501_INSTRUCTION_H
#define CIS501_INSTRUCTION_H
#include <iostream>
#include <string>
#include <stdint.h>
#include <cassert>
#include <vector>
#include <climits>
#include "trace.h"

using namespace std;

class Instruction {
	friend istream& operator>>(istream& in, Instruction& instrn);
	friend ostream& operator<<(ostream& out, Instruction& instrn);
	friend trace* extract_trace(const Instruction&);
public:
	Instruction() :
		 m_lnDoneCycle(ULONG_MAX), m_bIssued(false), m_bMispredicted(false){
	}
	~Instruction(){}

	bool IsValid() const {
		return m_strMicroOperation.size();
	}
	bool IsLoad() const {
		return m_cLoadStore == 'L';
	}
	bool IsStore() const {
		return m_cLoadStore == 'S';
	}
	bool IsLoadStore() const {
		return m_cLoadStore != '-';
	}
	bool IsCondBranch() const {
		return TargetPC() && m_cConditionRegister == 'R';
	}
	bool IsUCondBranch() const {
		return TargetPC() && m_cConditionRegister == '-';
	}
	bool HasIssued() const {
		return m_bIssued;
	}
	void markMispredict() {
		m_bMispredicted = true;
	}
	bool missPredicted() const {
		return m_bMispredicted;
	}
	bool IsDone(uint64_t nCurrentCycle) const{
			return (m_bIssued && m_lnDoneCycle <= nCurrentCycle);
		}
	bool IsNotDone(uint64_t nCurrentCycle) const {
		return !IsDone(nCurrentCycle);
	}

	uint64_t Age() const {
		return m_lnAge;
	}
	void setAge(uint64_t age) {
		m_lnAge = age + 1 ;
	}

	void Execute(uint64_t nCurrentCycle) {
		m_bIssued = true;
		m_lnIssueCycle = nCurrentCycle;
		m_lnDoneCycle = m_lnIssueCycle + m_nLatency;
	}

	int32_t MicroCount() const {
		return m_iMicroOpCount;
	}
	uint64_t PC() const {
		return m_lnInstructionAddress;
	}
	uint64_t TargetPC() const {
		return m_lnTargetAddressTakenBranch;
	}
	uint64_t MemAddr() const {
		return m_lnAddressForMemoryOp;
	}
	uint64_t FallthroughPC() const {
		return m_lnFallThroughPC;
	}
        bool DefactoBranch() const {
		return m_cTNnotBranch == 'T';
	}

        char TNnotBranch() const {
        	return m_cTNnotBranch;
        }

	void setFetchCycle(uint64_t nCycle) {
		m_lnFetchCycle = nCycle;
	}
	uint64_t FetchCycle() const {
		return m_lnFetchCycle;
	}
	uint64_t IssueCycle() const {
		return m_lnIssueCycle;
	}
	uint64_t DoneCycle() const {
		return m_lnDoneCycle;
	}
	void setCommitCycle(uint64_t nCycle) {
		m_lnCommitCycle = nCycle;
	}
	uint64_t CommitCycle() const {
		return m_lnCommitCycle;
	}
	uint32_t Latency() const {
		return m_nLatency;
	}

	int ArchSrcReg(int idx) const {
		assert(idx >= 1 && idx <= 3);
		return m_iArchSrcReg[idx - 1];
	}
	int ArchDestReg(int idx) const {
		assert(idx >= 1 && idx <= 2);
		return m_iArchDestReg[idx - 1];
	}
	int& PhySrcReg(int idx) {
		assert(idx >= 1 && idx <= 3);
		return m_iPhySrcReg[idx - 1];
	}
	int PhySrcReg(int idx) const {
		assert(idx >= 1 && idx <= 3);
		return m_iPhySrcReg[idx - 1];
	}
	int& PhyDestReg(int idx) {
		assert(idx >= 1 && idx <= 2);
		return m_iPhyDestReg[idx - 1];
	}
	int PhyDestReg(int idx) const {
		assert(idx >= 1 && idx <= 2);
		return m_iPhyDestReg[idx - 1];
	}
	int& PhyRegToFree(int idx) {
		assert(idx >= 1 && idx <= 2);
		return m_iPhyRegToFree[idx - 1];
	}
	int PhyRegToFree(int idx) const {
		assert(idx >= 1 && idx <= 2);
		return m_iPhyRegToFree[idx - 1];
	}

	const string& MicroName() const {
		return m_strMicroOperation;
	}
	const string& MacroName() const {
		return m_strMacroOperation;
	}

private:
	void initRegs();

private:
	// m_:  member variable
	// c:   char
	// i:   integer
	// n:   unsigned integer
	// l:   long
	// str: C++ string; use string::c_str() to get zero ended c style string
	int32_t m_iMicroOpCount;
	uint64_t m_lnInstructionAddress;
	int32_t m_iSourceRegister1;
	int32_t m_iSourceRegister2;
	int32_t m_iDestinationRegister;
	char m_cConditionRegister;
	char m_cTNnotBranch;
	char m_cLoadStore;
	int64_t m_liImmediate;
	uint64_t m_lnAddressForMemoryOp;
	uint64_t m_lnFallThroughPC;
	uint64_t m_lnTargetAddressTakenBranch;
	string m_strMacroOperation;
	string m_strMicroOperation;
	// variables indirectly generated (not read from the trace)
	int m_iPhySrcReg[3];
	int m_iPhyDestReg[2];
	int m_iArchSrcReg[3];
	int m_iArchDestReg[2];
	int m_iPhyRegToFree[2];
	// cycle tracking and bookkeeping
	uint64_t m_lnFetchCycle;
	uint64_t m_lnIssueCycle;
	uint64_t m_lnDoneCycle;
	uint64_t m_lnCommitCycle;
	uint32_t m_nLatency;
	// Other variables
	bool m_bIssued;
	uint64_t m_lnAge;
	bool m_bMispredicted;
};

#endif
