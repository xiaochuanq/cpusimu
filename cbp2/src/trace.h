// trace.h
// This file declares functions and a struct for reading trace files.

// these #define the Unix commands for decompressing gzip, bzip2, and
// plain files.  If they are somewhere else on your system, change these
// definitions.
#ifndef TRACE_H
#define TRACE_H

#define ZCAT            "/bin/gzip -dc"
#define BZCAT           "/usr/bin/bzip2 -dc"
#define CAT             "/bin/cat"

#include "branch.h"

struct trace {
	bool	taken;
	unsigned int target;
	branch_info bi;
};

void init_trace (char *);
trace *read_trace (void);
void end_trace (void);

#endif
