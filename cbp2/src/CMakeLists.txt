CMAKE_MINIMUM_REQUIRED(VERSION 2.5 FATAL_ERROR)

ADD_EXECUTABLE(predict predict.cc  tournament.cxx instruction.cxx)
ADD_EXECUTABLE(simulator simulator.cxx processor.h processor.cxx instruction.cxx gshare.h ogehl.hpp tournament.cxx)
#ADD_EXECUTABLE(psimulator psimulator.cxx processor.h processor.cxx instruction.cxx gshare.h ogehl.hpp tournament.cxx)
ADD_EXECUTABLE(test_counter testCounter.cpp)
ADD_EXECUTABLE(test_ogehl testOgehl.cpp)
#ADD_EXECUTABLE(predict ${PREDICT_SRC_LIST})
