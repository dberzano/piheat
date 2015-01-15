LDFLAGS+=-lwiringPi
CXXFLAGS+=-g -std=c++0x

rfSnifferPi: RFComm.o rfSnifferPi.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $+ -o $@
