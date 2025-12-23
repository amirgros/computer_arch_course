CXX = g++
CXXFLAGS = -g -Wall -std=c++11

cacheSim: cacheSim.o
	$(CXX) $(CXXFLAGS) -o cacheSim cacheSim.o

cacheSim.o: cacheSim.cpp cache.h
	$(CXX) $(CXXFLAGS) -c cacheSim.cpp

.PHONY: clean
clean:
	rm -f *.o
	rm -f cacheSim