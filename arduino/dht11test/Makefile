LDFLAGS+=-lwiringPi
CXXFLAGS+=-g -std=c++0x -Wall

rfSnifferPi: RFComm.o rfSnifferPi.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $+ -o $@
