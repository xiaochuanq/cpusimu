// tournament.h
// This file contains a tournament predictor.
#ifndef TOURNAMENT_H
#define TOURNAMENT_H

#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "branch.h"
#include "trace.h"
#include "predictor.h"
#include "instruction.h"

struct table {
  int64_t size;             /* Number of counters */
  int64_t bits;             /* log(# of counters) */
  int8_t counters[1048576]; /* Maximum number of counters */
  /* For gshare */
  int32_t hist;             /* History of branches */
  int32_t hist_mask;        /* Bitmask for hist */
};

struct tournament {
  struct table gshare;
  struct table bimodal;
  struct table chooser;
};


class tournament_predictor : public branch_predictor {
 public:
  branch_update u;
  tournament_predictor(int64_t size);
  branch_update *predict (const branch_info &) {assert(false);}; /* Must pass instruction*/
  branch_update *predict (const branch_info &, const Instruction &);
  void update(branch_update *u, bool taken, unsigned int target);
 private:
  struct tournament trnmnt;
  int64_t _totalCondBranches, _totalCorrect;
};

#endif
