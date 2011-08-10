#include <cassert>
#include "instruction.h"

istream& operator >>(istream& in, Instruction& instrn) {
	in >> instrn.m_iMicroOpCount >> hex >> instrn.m_lnInstructionAddress;
	in >> dec >> instrn.m_iSourceRegister1 >> instrn.m_iSourceRegister2;
	in >> instrn.m_iDestinationRegister >> instrn.m_cConditionRegister;
	in >> instrn.m_cTNnotBranch >> instrn.m_cLoadStore >> instrn.m_liImmediate;
	in >> hex >> instrn.m_lnAddressForMemoryOp >> hex
			>> instrn.m_lnFallThroughPC;
	in >> hex >> instrn.m_lnTargetAddressTakenBranch;
	in >> instrn.m_strMacroOperation >> instrn.m_strMicroOperation;
        instrn.initRegs();
        instrn.m_nLatency = instrn.m_cLoadStore == 'L' ? 3 : 1;
	return in;
}

ostream& operator<<(ostream& out, Instruction& instrn) {
	out << instrn.m_lnAge<< ": ";
	out << instrn.m_lnFetchCycle << " " << instrn.m_lnIssueCycle << " ";
	out << instrn.m_lnDoneCycle  << " " << instrn.m_lnCommitCycle;
	for (int i = 1; i <= 3; ++i) {
		int areg = instrn.ArchSrcReg(i);
		if (areg >= 0) {
			out << ", r" << instrn.ArchSrcReg(i) << " -> p"
					<< instrn.PhySrcReg(i);
		}
	}
	for (int i = 1; i <= 2; ++i) {
		int areg = instrn.ArchDestReg(i);
		if (areg >= 0) {
			out << ", r" << instrn.ArchDestReg(i) << " -> p"
					<< instrn.PhyDestReg(i);
			out << " [p" << instrn.PhyRegToFree(i) << "]";
		}
	}
	out << " | " << instrn.MacroName() << " " << instrn.MicroName() << endl;
	return out;
}

void Instruction::initRegs() {
	m_iArchSrcReg[0] = m_iSourceRegister1;
	m_iArchSrcReg[1] = m_iSourceRegister2;
	m_iArchSrcReg[2] = m_cConditionRegister == 'R' ? 49 : -1;
	m_iArchDestReg[0] = m_iDestinationRegister;
	m_iArchDestReg[1] = m_cConditionRegister == 'W' ? 49 : -1;
	m_iPhyRegToFree[0] = m_iPhyRegToFree[1] = -1;
	m_iPhySrcReg[0] = m_iPhySrcReg[1] = m_iPhySrcReg[2] = -1;
	m_iPhyDestReg[0] = m_iPhyDestReg[1] = -1;
}

trace* extract_trace(const Instruction& instrn)
{
  static trace t;
  t.taken =( instrn.m_cTNnotBranch != 'N');
  //if it's a conditional branch, it is taken only if 'N';
  //otherwise if it is a unconditional branch, it is always taken;
  //and if it is not a branch at all, this flag is meaningless.
  t.target = instrn.TargetPC();
  t.bi.address = instrn.PC();
  t.bi.br_flags = instrn.IsCondBranch() ? BR_CONDITIONAL :
    (instrn.IsUCondBranch()? BR_UNCONDITIONAL : BR_NOTABRANCH);
  t.bi.opcode = t.bi.br_flags == BR_CONDITIONAL ? OP_JNZ : OP_NA;//n/a means we don't care;
  return &t;
}
