#include "processor.h"
#include "predictor.h"
#include "instruction.h"

Processor::Processor(int nArchReg, int nPhyReg, uint32_t nRobDepth, uint32_t nWidth):
    m_nRobCapacity(nRobDepth), m_nNumArchReg(nArchReg), m_nNumPhyReg(nPhyReg),
    m_nWidth(nWidth), m_maptable(nArchReg, 0), m_freelist(nPhyReg
                                                          - nArchReg, 0), m_scoreboard(nPhyReg, 0), m_nCurrentCycle(
                                                                                                                    0), m_nTotalMacroops(0), m_nTotalMicroops(0)
{
    for (unsigned int i = 0; i < m_maptable.size(); ++i)
        m_maptable[i] = i;
    for (unsigned int i = 0; i < m_freelist.size(); ++i)
        m_freelist[i] = nArchReg + i;
    m_rob.clear();
}

void Processor::reset() {
    m_nCurrentCycle = m_nTotalMicroops = m_nTotalMacroops = 0;
    m_rob.clear();
    for (unsigned int i = 0; i < m_maptable.size(); ++i)
        m_maptable[i] = i;
    for (unsigned int i = 0; i < m_freelist.size(); ++i)
        m_freelist[i] = m_nNumArchReg + i;
    for(unsigned int i = 0; i < m_scoreboard.size();++i)
        m_scoreboard[i] = 0;
}

Processor::~Processor() {
}

Instruction* Processor::readInstruction(istream& ins) {
    m_rob.push_back(Instruction());
    Instruction& instrn = m_rob.back();
    if(ins){
        ins >> instrn; // read trace from input flow to an instruction object
        if (!instrn.IsValid()) {
            m_rob.pop_back(); // delete invalid read-ins
            return 0; // sometimes the trace file has empty ending lines
            // so that the
        }
    }
    else{
        m_rob.pop_back();
        return 0;
    }
    // file ins not at EOF but the instruction read in ins invalid.
    instrn.setAge(m_nTotalMicroops);
    instrn.setFetchCycle(currentCycle());
    if (instrn.MicroCount() == 1)
        ++m_nTotalMacroops;
    ++m_nTotalMicroops;
    return &instrn;
}

void Processor::renameInstruction(Instruction& instrn) {
    for (int i = 1; i <= 3; ++i) // source registers
	{
            int areg = instrn.ArchSrcReg(i);
            assert(areg < m_nNumArchReg);
            if (areg >= 0) // valid in this instruction
                instrn.PhySrcReg(i) = m_maptable[areg];
	}
    for (int i = 1; i <= 2; ++i) // destination registers
	{
            int areg = instrn.ArchDestReg(i);
            assert(areg < m_nNumArchReg);
            if (areg >= 0) {
                instrn.PhyRegToFree(i) = m_maptable[areg];
                int new_preg = m_freelist.front();
                m_freelist.pop_front();
                m_maptable[areg] = new_preg;
                instrn.PhyDestReg(i) = m_maptable[areg];
            }
	}
}

void Processor::unReady(const Instruction& instrn) {
    // 4. for each valid destination physical register
    //    set the register as "not ready" by setting the
    //    correspond score board entry to -1.
    for (int r = 1; r <= 2; ++r) {
        if (instrn.PhyDestReg(r) >= 0)
            m_scoreboard[instrn.PhyDestReg(r)] = -1;
    }
}

void Processor::fetch(istream& ins) {
    if(!ins)
        return;
    for (uint32_t i = 0; i < width(); ++i) {
        if (m_rob.size() == robCapacity())
            return;// ROB is full
        Instruction* pinstrn = readInstruction(ins);
        if (!pinstrn) //trace ends
            return;
        renameInstruction(*pinstrn);
        unReady(*pinstrn);
    }
}

void Processor::commit(ostream& out) {
    for (uint32_t i = 0; i < width(); ++i) {
        if ( !m_rob.empty() && m_rob.front().IsDone(currentCycle() ) ) {
            Instruction& instrn = m_rob.front();
            instrn.setCommitCycle(currentCycle());
	    //       out << instrn; // dump trace
            for (int r = 1; r <= 2; ++r) // free allocated physical destination reg
                {
                    if (instrn.PhyRegToFree(r) >= 0) // if valid
                        m_freelist.push_back(instrn.PhyRegToFree(r));
                }
            m_rob.pop_front(); //dequeue
        }
    }
}

void Processor::issue() {
    unsigned int nCnt = 0;
    for (ROB::iterator rit = m_rob.begin(); rit != m_rob.end(); ++rit) {
        if (!rit->HasIssued() && isReady(*rit)) {
            rit->Execute(currentCycle()); // set issued as true, and set issue cycle as current
            // and set done cycle as current + latency
            for (int r = 1; r <= 2; ++r) // set each destination physical register
                {
                    int ridx = rit->PhyDestReg(r);
                    if( ridx >= 0)
                        m_scoreboard[ ridx ] = rit->Latency();
                }
            if (++nCnt == width())
                break;
        }
    }
}

void Processor::advance() {
    ++m_nCurrentCycle;
    for (vector<int>::iterator regit = m_scoreboard.begin(); regit
             != m_scoreboard.end(); ++regit) {
        if ((*regit) > 0)
            --(*regit);
    }
}

bool Exp1Processor::isReady(const Instruction& instrn) const {
    for (int r = 1; r <= 3; ++r)
        if (instrn.PhySrcReg(r) >= 0 && m_scoreboard[instrn.PhySrcReg(r)])
            return false;
    return true;
}

bool Exp2Processor::isReady(const Instruction& instrn) const {
    if( !Exp1Processor::isReady(instrn) )
        return false;
    if (instrn.IsLoad()) {
        for (ROB::const_iterator rit = m_rob.begin(); rit != m_rob.end(); ++rit) {
            if (rit->IsStore() && rit->Age() < instrn.Age() && rit->IsNotDone(
                                                                              currentCycle()))
                return false;
        }
    }
    return true;
}

bool Exp3Processor::isReady(const Instruction& instrn) const {
    if( !Exp1Processor::isReady(instrn) )
        return false;
    if (instrn.IsLoad()) {
        for (ROB::const_iterator rit = m_rob.begin(); rit != m_rob.end(); ++rit) {
            if (rit->IsStore() && rit->Age() < instrn.Age() && rit->IsNotDone(
                                                                              currentCycle()))
                if (rit->MemAddr() == instrn.MemAddr())
                    return false;
        }
    }
    return true;
}

void Exp4Processor::issue() {
    unsigned int nCnt = 0;
    for (ROB::iterator rit = m_rob.begin(); rit != m_rob.end(); ++rit) {
        if (!rit->HasIssued() && isReady(*rit)) {
            rit->Execute(currentCycle()); // set issued as true, and set issue cycle as current
            // and set done cycle as current + latency
            for (int r = 1; r <= 2; ++r){ // set each destination physical register
                int ridx = rit->PhyDestReg(r);
                if( ridx >= 0)
                    m_scoreboard[ ridx ] = rit->Latency();
            }
            if (rit->missPredicted()) { // stall fetching if mis-predicted
                assert(m_nFetchReady < 0);
                m_nFetchReady = rit->Latency() + 4;
            }
            if (++nCnt == width())
                break;
        }
    }
}

void Exp4Processor::fetch(istream& ins) {
    if (!ins )
        return;
    for (unsigned int i = 0; i < width(); ++i) {
        if (m_nFetchReady) {
            --m_nFetchReady;
            break;
        }
        else {
            if (m_rob.size() == robCapacity())
                break;

            Instruction* pinstrn = readInstruction(ins);
            if (!pinstrn)
                return;
            renameInstruction(*pinstrn);
            unReady(*pinstrn);

	    trace * t = extract_trace(*pinstrn);
            branch_update *bu = m_pPred->predict(t->bi, *pinstrn);
            if (t->bi.br_flags & BR_CONDITIONAL) {
	      if( bu->direction_prediction() != t->taken){
	      //         if (bu->direction_prediction() != pinstrn->DefactoBranch()){
                    pinstrn->markMispredict();
                    m_nFetchReady = -1; //suspend fetch
                }
	    }
                //m_predictor.update(pinstrn->DefactoBranch(), pinstrn->TargetPC());//update the predictor
            m_pPred->update(bu, t->taken, t->target, *pinstrn);
        }
    }
}
