CXX = g++
CXXFLAGS = -g -Wall -std=c++11

cacheSim: cacheSim.o cache.o
	$(CXX) $(CXXFLAGS) -o cacheSim cacheSim.o cache.o

cacheSim.o: cacheSim.cpp cache.h
	$(CXX) $(CXXFLAGS) -c cacheSim.cpp

cache.o: cache.cpp cache.h
	$(CXX) $(CXXFLAGS) -c cache.cpp

.PHONY: clean
clean:
	rm -f *.o
	rm -f cacheSim