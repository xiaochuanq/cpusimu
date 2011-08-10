#ifndef TOURNAMENT_C
#define TOURNAMENT_C
#define __STDC_FORMAT_MACROS 1

#include "tournament.h"

enum counter {STRONG_NO, WEAK_NO, WEAK_YES, STRONG_YES};

static void table_init(struct table *t, int64_t bits, int64_t hist_length) {
    t->bits = bits;
    t->size = 1 << bits;
    t->hist = 0;
    t->hist_mask = (1 << hist_length) -1;
}

/* Return a string representation of the counters */
static char* counter_str(struct table *t) {
    assert(t->size <= 128);
    static char s[129];
    int32_t j;
    for (j = 0; j < t->size; j++) {
        *(s+j) = "NntT"[t->counters[j]];
    }
    *(s+j) = '\0';
    return s;
}

static char* hist_str(struct table *t) {
    assert(t->hist_mask < 128);
    static char s[129];
    char *m = s;

    for (int32_t j = log2(t->hist_mask+1)-1; j >= 0; j--) {
        *m++ = "NT"[(t->hist >> j) & 1];
    }
    *m = '\0';
    return s;
}

static char* choice_str(struct table *t) {
    assert(t->size <= 128);
    static char s[129];
    int32_t j;
    for (j = 0; j < t->size; j++) {
        *(s+j) = "BbgG"[t->counters[j]];
    }
    *(s+j) = '\0';
    return s;
}

static char* bimodal_str(struct table *t) {
    static char s[40];
    sprintf(s, "(Bimodal %2" PRIi64 " count bits)", t->bits);
    return s;
}

static char* gshare_str(struct table *t) {
    static char s[80];
    int32_t h = log2(t->hist_mask+1);
    sprintf(s, "(Gshare %2" PRIi64 " count bits, %2" PRIi32 " hist)", t->bits, h);
    return s;
}

static char* tournament_str(struct tournament *t) {
    static char s[140], c[80];

    sprintf(c, "(Tournament %2" PRIi64 " count bits)", t->chooser.bits);
    sprintf(s, "%s %s %s", c, bimodal_str(&t->bimodal), gshare_str(&t->gshare));
    return s;
}

static char bimodal_predictor(const Instruction *op, struct table *t) {
    /* Make prediction */
    int64_t mask = (1 << t->bits)-1;
    int64_t offset = (op->PC() & mask);
    assert(offset < t->size);
    char taken = (t->counters[offset] >= WEAK_YES) ? 'T' : 'N';

    /* Update */
    int8_t val = t->counters[offset] + ((op->DefactoBranch()) ? 1 : -1);
    t->counters[offset] = fmin(fmax(val, STRONG_NO), STRONG_YES);

    return taken;
}

static char gshare_predictor(const Instruction *op, struct table *t) {
    /* Make predicition */
    int64_t pc_mask = (1 << t->bits) - 1;
    int64_t offset = (op->PC() ^ t->hist) & pc_mask;

    assert(offset < t->size);
    char taken = (t->counters[offset] >= WEAK_YES) ? 'T' : 'N';

    /* Update */
    int8_t val = t->counters[offset] + ((op->DefactoBranch()) ? 1 : -1);
    t->counters[offset] = fmin(fmax(val, STRONG_NO), STRONG_YES);
    t->hist = ((t->hist << 1) | (op->DefactoBranch() ? 1 : 0)) & t->hist_mask;

    return taken;
}

static char tournament(const Instruction *op, struct tournament *t) {
    struct table *c = &(t->chooser);

    /* Make prediction */
    char gshare = gshare_predictor(op, &t->gshare);
    char bimodal = bimodal_predictor(op, &t->bimodal);

    int64_t pc_mask = (1 << c->bits) - 1;
    int64_t offset = (op->PC() & pc_mask);
    assert(offset < c->size);

    char p = (c->counters[offset] >= WEAK_YES) ? gshare : bimodal;

    /* Update */
    if (gshare != bimodal) {
        int8_t val = c->counters[offset] + ((op->TNnotBranch() == gshare) ? 1 : -1);
        c->counters[offset] = fmin(fmax(val, STRONG_NO), STRONG_YES);
    }

    return p;
}


tournament_predictor::tournament_predictor(int64_t size) {
    u.direction_prediction(true);
    table_init(&(trnmnt.gshare), size-1, size-1);
    table_init(&(trnmnt.bimodal), size-2, 0);
    table_init(&(trnmnt.chooser), size-2, 0);
    _totalCondBranches = 0;
    _totalCorrect = 0;
}

branch_update *tournament_predictor::predict(const branch_info & b, const Instruction &insn) {
    if (!insn.IsCondBranch()) {
        return &u;
    }

    if (_debug) {
        struct tournament *t = &trnmnt;
        fprintf(stdout, "%s %s ",
                choice_str(&t->chooser), counter_str(&t->bimodal));
        fprintf(stdout, "%s  %s | %" SCNx64 "  %c | ",
                counter_str(&t->gshare), hist_str(&t->gshare),
                insn.PC(), insn.TNnotBranch());
    }

    char taken = tournament(&insn, &trnmnt);
    u.direction_prediction((taken == 'T') ? true : false);
    int c = (taken == insn.TNnotBranch()) ? 1 : 0;

    _totalCorrect += c;
    _totalCondBranches++;

    if (_debug) {
        fprintf(stdout, "%c  %-10s %" SCNi64 "\n",
                taken, c ? "correct" : "incorrect",
                (_totalCondBranches - _totalCorrect));
    }
    return &u;
}

void tournament_predictor::update(branch_update *u, bool taken, unsigned int target) {

    }
#endif
