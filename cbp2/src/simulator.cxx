//#define __STDC_FORMAT_MACROS 1
#include <vector>
#include <deque>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string.h>
#include <string>
#include "processor.h"
#include "gshare.h"
#include "ogehl.hpp"
#include "tournament.h"

using namespace std;

//// Gloabl Variables
int NumArchReg() {
	static int nNumArchReg = 50;
	return nNumArchReg;
}

branch_predictor*  PredictorPtr = 0;
/*((){
  static branch_predictor* pred_ptr = 0;
  return pred_ptr;
  }*/

void simulate(istream& ins, ostream& outs, int nNumArchReg, int nNumPhyReg,
		int nRobDepth, int N, int exp);

string params;

//// Program Entrance <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
int main(int argc, char *argv[]) {
	if (argc < 6) {
		cout << "Usage: " << argv[0] << " "
				<< "trace_file num_physics_reg rob_depth issue_width experiment_id/predictor_name [output_file]"
				<< endl;
		return 1;
	}

	/*	ifstream infile(argv[1]);
	if (!infile) {
		cout << "Unable to open trace document or empty trace" << endl;
		return -1;
		}*/

	int nNumPhyReg = atoi(argv[2]);
	int nRobDepth = atoi(argv[3]);
	int N = atoi(argv[4]);
	int exp = atoi(argv[5]);
	params = string( argv[2]) +"-" + string(argv[3]) + "-" + string(argv[4])+"-"+string(argv[5]);
	/*
	  ss >> num_tables >> idx_bits >> l1_sz >> alpha >> theta >>  ctr_bits;
	ss >> addtl_hist_length_vals >> thresh_bits >> a_ctr_bits;
	ss >> dyn_theta >> dyn_histl >> verbose;

	int32_t ttl_lengths = num_tables+addtl_hist_length_vals;
	Lengths *l = new Lengths(l1_sz, alpha, ttl_lengths);

	std::vector<Table *> *tables =				\
	    makeTables(*l, idx_bits, num_tables, addtl_hist_length_vals, ctr_bits);

	ogehl_predictor *o = new ogehl_predictor(theta, tables, idx_bits,
						thresh_bits, a_ctr_bits,
						dyn_theta, dyn_histl, verbose);

	 ogehl_predictor(int32_t theta, std::vector<Table *> *tables, int32_t idx_bits,
                    int32_t thresh_bits, int32_t a_ctr_bits,
                    bool dynamic_theta, bool dynamic_hist_length,
                    bool verbose=false)  
	   gshare( int history_len, int log_table_len);
	*/
	
	if(exp <= 0 || exp > 3  ){
	  exp = 4;
	  if(  strcmp(argv[5], "gshare")  == 0)
	    PredictorPtr = new gshare_predictor(15,15);
	  else if( strcmp( argv[5], "tournament") == 0)
	    PredictorPtr = new tournament_predictor(15);
	  else if( strcmp( argv[5], "gehl") == 0){
	    /* tables=6 ind= 12 L1=2 alpha=2 theta=12 cnterbit=4 add=0 tcbit=1, ac=1*/
	    Lengths len( 2, 2.0, 6+0);
	    vector<Table*> *tables = makeTables(len, 12, 6, 0, 4);
	    PredictorPtr = new ogehl_predictor(12, tables, 12,
						 1, 1, false, false,false);
	  }
	  else if( strcmp( argv[5], "ogehl") == 0){
	    /*tables=6, ind=12 L1=2, alpha=2, theta=12 cnterbit=4, add=2, tc=7, ac=9*/
	    Lengths len(2, 2.0, 6+2);
	    vector<Table*> *tables = makeTables(len, 12, 6, 2, 4);
	    PredictorPtr = new ogehl_predictor(12, tables, 12,
						 7, 9, true, true, false );
	  }
	  else{
	    cout <<"Experiment id shoudl be an integer in 1 ~ 3 or a predictor name of gshare, gehl, ogehl, or tournament.";
	    return -1;
	  }
	}
	
	ofstream outfile;
        if(strcmp(argv[1],"stdin")!=0){
	  ifstream infile(argv[1]);
	  if (!infile) {
		cout << "Unable to open trace document or empty trace" << endl;
		return -1;
	  }
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
	delete PredictorPtr;
       return 0;
}

void simulate(istream& ins, ostream& outs, int nNumArchReg, int nNumPhyReg,
		int nRobDepth, int N, int exp) {
        Processor * pp;
	outs << params<<endl;
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
	    pp = new Exp4Processor(nNumArchReg, nNumPhyReg, nRobDepth, N, PredictorPtr);
	}
	if (!pp) {
		outs << "Failed to make an experiment" << endl;
		exit(-1);
	}
	outs << "Processing trace..." << endl; // Start tracing
	try {
		do {
		  if(pp->totalMicro()% 1000000 == 0)
			  cout << ">";
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
