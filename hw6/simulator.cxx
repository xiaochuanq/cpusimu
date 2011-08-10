//#define __STDC_FORMAT_MACROS 1
#include <vector>
#include <deque>
#include <iostream>
#include<istream>
#include <fstream>
#include <cstdlib>
#include "processor.h"
using namespace std;

//// Gloabl Variables
int NumArchReg() {
	static int nNumArchReg = 50;
	return nNumArchReg;
}

void simulate(istream& ins, ostream& outs, int nNumArchReg, int nNumPhyReg,
		int nRobDepth, int N, int exp);

//// Program Entrance <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
int main(int argc, char *argv[]) {
	if (argc < 6) {
		cout << "Usage: " << argv[0] << " "
				<< "trace_file num_physics_reg rob_depth issue_width experiment_id [output_file]"
				<< endl;
		return 1;
	}

	ifstream infile(argv[1]);
        
	
	/*if (!infile) {
		cout << "Unable to open trace document or empty trace" << endl;
		return -1;
	}*/

	int nNumPhyReg = atoi(argv[2]);
	int nRobDepth = atoi(argv[3]);
	int N = atoi(argv[4]);
	int exp = atoi(argv[5]);
	
	ofstream outfile;
       if(strcmp(argv[1],"stdin")!=0){
	if (argc >= 7)
		outfile.open(argv[6], ofstream::out | ofstream::trunc);

	if (argc >= 7 && outfile) {
		simulate(infile, outfile, NumArchReg(), nNumPhyReg, nRobDepth, N, exp);
		outfile.close();
	} else
		simulate(infile, cout, NumArchReg(), nNumPhyReg, nRobDepth, N, exp);

      }
       else{
        if (argc >= 7)
		outfile.open(argv[6], ofstream::out | ofstream::trunc);

	if (argc >= 7 && outfile) {
		simulate(cin, outfile, NumArchReg(), nNumPhyReg, nRobDepth, N, exp);
		outfile.close();
	} else
		simulate(cin, cout, NumArchReg(), nNumPhyReg, nRobDepth, N, exp);

       }
		return 0;
}

void simulate(istream& ins, ostream& outs, int nNumArchReg, int nNumPhyReg,
		int nRobDepth, int N, int exp) {
	Processor* pp = 0;
	switch (exp) { // different experiment uses different processors
	case 1: // Experiment #1 - Unrealistic Best-Case ILP
		pp = new Exp1Processor(nNumArchReg, nNumPhyReg, nRobDepth, N);
		break;
	case 2: // Experiment #2 - Conservative Memory Scheduling
		pp = new Exp2Processor(nNumArchReg, nNumPhyReg, nRobDepth, N);
		break;
	case 3: // Experiment #3 - Perfect Memory Scheduling
		pp = new Exp3Processor(nNumArchReg, nNumPhyReg, nRobDepth, N);
		break;
	default: // Experiment #4 - Realistic Branch Prediction
		pp = new Exp4Processor(nNumArchReg, nNumPhyReg, nRobDepth, N);
	}
	if (!pp) {
		outs << "Failed to make an experiment" << endl;
		exit(-1);
	}
	outs << "Processing trace..." << endl; // Start tracing
	try {
		do {
			// To prevent an instruction from flowing through the pipeline in a
			// single cycle, we perform the pipeline stages in reverse order.
			////---------- Step#1: Commit--------------------------
			pp->commit(outs);
			////---------- Step#2: Issue---------------------------------------
			pp->issue();
			////---------- Step #3: Fetch & Rename------------------------------------
			pp->fetch(ins);
			////---------- Step #4: Advance to the Next Cycle------------------------
			pp->advance();
			//------------
		} while (!pp->pipeline_empty() || ins);
		outs << "Processed " << pp->totalMicro() << " trace records." << endl;
		outs << "Micro-ops: " << pp->totalMicro() << endl;
		outs << "Macro-ops: " << pp->totalMacro() << endl;
		outs << "TotalCycles: " << pp->currentCycle() << endl;
		outs << "uIPC: " << float(pp->totalMicro()) / pp->currentCycle()
				<< endl;
	} catch (...) {
		cout << "Error parsing trace at line " << pp->totalMicro() << endl;
	}
}
