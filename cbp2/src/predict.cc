// predict.cc
// This file contains the main function.  The program accepts a single
// parameter: the name of a trace file.  It drives the branch predictor
// simulation by reading the trace file and feeding the traces one at a time
// to the branch predictor.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include "branch.h"
#include "trace.h"
#include "predictor.h"

#include "gehl.h"
#include "ogehl.hpp"
#include "bc2.h"
#include "tournament.h"
#include "gshare.h"
#include "seznec_ogehl.h"

#ifndef CBP2
#include <iostream>
#include <fstream>
#include "instruction.h"
using namespace std;
#endif


template<typename T>
struct delete_ptr{
  void operator()(T* ptr){ delete ptr;}
};

void readOgehlConfig(std::string fname, std::vector<branch_predictor *> *p) {
    ifstream in(fname.c_str());

    if (!in) {
        cerr << "Error reading from file: " << fname << endl;
        exit(1);
    }

    // GEHL parameters
    int32_t theta, l1_sz;
    double alpha;
    int32_t num_tables, idx_bits, ctr_bits;

    // OGEHL parameters
    int32_t addtl_hist_length_vals;
    int32_t thresh_bits;
    int32_t a_ctr_bits;

    // Debug parameters
    bool dyn_theta, dyn_histl, verbose;

    std::string s;
    while (getline(in, s)) {
        if (s[0] == '#') {
            continue;
        }
        std::stringstream ss;
        ss.clear();
        ss.str(s);

	// GEHL, OGEHL, debug
        ss >> num_tables >> idx_bits >> l1_sz >> alpha >> theta >>  ctr_bits;
	ss >> addtl_hist_length_vals >> thresh_bits >> a_ctr_bits;
	ss >> dyn_theta >> dyn_histl >> verbose;

	// Build it!
	int32_t ttl_lengths = num_tables+addtl_hist_length_vals;
	Lengths *l = new Lengths(l1_sz, alpha, ttl_lengths);

	std::vector<Table *> *tables =				\
	    makeTables(*l, idx_bits, num_tables, addtl_hist_length_vals, ctr_bits);

	ogehl_predictor *o = new ogehl_predictor(theta, tables, idx_bits,
						thresh_bits, a_ctr_bits,
						dyn_theta, dyn_histl, verbose);

        printf("//============================== New Predictor ==============================//\n");
        printf("theta alpha L1 Tables Index cnterbit       size log(size)\n");
        printf("%5d %3.2f %2d %7d %5d %8d %10d %4.2f\n",
               theta, alpha, l1_sz, num_tables, idx_bits, ctr_bits, o->bitSize(), log2(o->bitSize()));

	p->push_back(o);
	printf("\n%10s %3s %3s %9s %9s\n",
	       "addtl_hist", "TC", "AC", "Dyn Theta", "Dyn Hist");
	printf("%10d %3d %3d %9d %9d\n\n",
	       addtl_hist_length_vals, thresh_bits, a_ctr_bits, dyn_theta, dyn_histl);
	//cout << l->debugString();
	//cout << (o)->debugString();
    }
    printf("//===========================================================================//\n");
}

int main (int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <filename>.trace <predictor> <debug/release> [predictor_param]\n", argv[0]);
        exit (1);
    }

    // open the trace file for reading
    istream *ins;
    if (strcmp(argv[1], "stdin") == 0) {
        ins =&cin;
    } else {
        ins = new ifstream(argv[1]);
    }

    // initialize competitor's branch prediction code
    std::vector<branch_predictor *> predictors;
    /*------------------------------------------------*/
    if (strcmp(argv[2], "bc2") == 0) {
        assert(argc >= 5);

        for (int32_t i = 4; i < argc; i++) {
            branch_predictor *p = new bc2_predictor(atoi(argv[i]));
            predictors.push_back(p);
        }
    }
    /*------------------------------------------------*/
    else if (strcmp(argv[2], "tournament") == 0) {
        assert(argc >= 5);

        for (int32_t i = 4; i < argc; i++) {
            branch_predictor *p = new tournament_predictor(atoi(argv[i]));
            predictors.push_back(p);
        }
    }
    /*--------------------GSHARE----------------------*/
    else if (strcmp(argv[2], "gshare") == 0) {
        assert(argc >= 5);

        for (int32_t i = 4; i < argc; i++) {
            branch_predictor *p = new gshare_predictor(atoi(argv[i]), atoi(argv[i]));
            predictors.push_back(p);
        }
    }
    /*------------------------------------------------*/
    else if (strcmp(argv[2], "gehl") == 0) {
        branch_predictor *p = NULL;
        if( argc >= 10)
	    p = new gehl_predictor(atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atof(argv[7]), atoi(argv[8]), atoi(argv[9]));
        else
	    p = new gehl_predictor();
        predictors.push_back(p);
    }
    /*------------------------------------------------*/
    else if (strcmp(argv[2], "ogehl") == 0) {
        assert(argc >= 5);
        std::string fname(argv[4]);
        readOgehlConfig(fname, &predictors);
    }
    /*------------------------------------------------*/
    else if(strcmp(argv[2], "seznec") == 0){
        predictors.push_back(new seznec_ogehl_predictor());
    }

    //=========================== Error checking ============================//
    if (predictors.size() == 0) {
        cout << "Specify a predictor";
        exit(1);
    }
    for (unsigned int i = 0; i < predictors.size(); i++) {
        if (predictors.at(i) == NULL) {
            cerr << "NULL predictor in predictors list! Index: " <<  i << endl;
            exit(1);
        }
    }

    //================================ Debug ================================//
    bool debug = (argv[3][0] == 'd' || argv[3][0] == 'D');

    for (unsigned int i = 0; i < predictors.size(); i++) {
        predictors.at(i)->setDebug(debug);
        predictors.at(i)->printConfig();
    }

    //============================== Main loop ==============================//
    // some statistics to keep
    int64_t totalMicroops=0, branches=0, condBranches=0;

    // number of direction mispredictions
    std::vector<int64_t> dmiss(predictors.size(), 0);
    // Number of correct predictions
    std::vector<int64_t> totalCorrect(predictors.size(), 0);

    // keep looping until end of file
    for (;;) {

        // get a trace
        Instruction instrn;
        (*ins) >> instrn;
        if(!instrn.IsValid()) {
            if(ins)
                cerr << "Invalid instruction!" << endl;
            break;
        }
        totalMicroops++;
        if (totalMicroops % 1000000L == 0) {
            cerr << "Processed " << totalMicroops/1000000L<< "M microops" << endl;
        }
        if (instrn.IsCondBranch() || instrn.IsUCondBranch()) {
            branches++;
        }

        trace * t = extract_trace(instrn);
        if (t->bi.br_flags & BR_CONDITIONAL) {
            condBranches++;
        }

        for (unsigned int i = 0; i < predictors.size(); i++) {
            branch_predictor *p = predictors.at(i);
            int64_t dm = dmiss.at(i);
            int64_t tc = totalCorrect.at(i);

            branch_update *u = p->predict(t->bi, instrn);

            // collect statistics for a conditional branch trace
            if (t->bi.br_flags & BR_CONDITIONAL) {
                // count a direction misprediction
                tc += (u->direction_prediction () == t->taken) ? 1 : 0;
                dm += (u->direction_prediction () == t->taken) ? 0 : 1;
            }

            p->update (u, t->taken, t->target, instrn);
            dmiss[i] = dm;
            totalCorrect[i] = tc;
        }
    }

    cout << "Total microops: " << std::dec<< totalMicroops << endl;
    cout << "Total branches: " << branches << endl;
    cout << "Total cond. branches: " << condBranches << endl;

    for (unsigned int i = 0; i < predictors.size(); i++) {
	cout << "Predictor #" << i << endl;
        cout << "Total correct pred:   " << totalCorrect.at(i) << endl;
        printf("Total correct pct: %2.2f\n", ((double)(100*totalCorrect.at(i)) / condBranches));
        printf ("%0.3f MPKI\n", 1000.0 * (dmiss.at(i) / 1e8));
    }

    for_each( predictors.begin(), predictors.end(), delete_ptr<branch_predictor>());

    return 0;
}
