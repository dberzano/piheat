LDFLAGS+=-lwiringPi
CXXFLAGS+=-g -std=c++0x -Wall

all: rfSnifferPi rfTemp

rfSnifferPi: RFComm.o rfSnifferPi.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $+ -o $@

rfTemp: RFComm.o rfTemp.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $+ -o $@
