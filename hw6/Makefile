# Make file of CIS501 Final Homework

CXXFLAG = -O3
DEBUGFLAG = -g -Wall -O0

all: debug

release : simulator.cxx processor.h processor.cxx  gshare.h instruction.h instruction.cxx
	g++ $(CXXFLAG) simulator.cxx processor.h processor.cxx  gshare.h instruction.h instruction.cxx \
        -o simulator 

debug : simulator.cxx processor.h processor.cxx  gshare.h instruction.h instruction.cxx
	g++ $(DEBUGFLAG) simulator.cxx processor.h processor.cxx  gshare.h instruction.h instruction.cxx \
	-o simulator

clean: 
	rm -rf *o ~* *bak *~ *gch simulator
