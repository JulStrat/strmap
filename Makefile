CC  = gcc -ansi -pedantic
CXX = c++ -std=c++11 -O2 -DNEDEBUG
#CXXFLAGS = -m32 -Wall -Wextra -Wconversion -Wshadow
CXXFLAGS = -Wall -Wextra -Wconversion -Wshadow

all: bench words robin_hood phmap test

test: tests/test.c strmap.c
	$(CC) -g $(CXXFLAGS) -o test -I. -Itests tests/test.c strmap.c

robin_hood: robin_hood.o strmap.o
	$(CXX) $(CXXFLAGS) -o robin_hood robin_hood.o strmap.o

robin_hood.o: benchs/robin_hood.cc
	$(CXX) -c $(CXXFLAGS) -o robin_hood.o -I. -Ibenchs benchs/robin_hood.cc

phmap: phmap.o strmap.o
	$(CXX) $(CXXFLAGS) -o phmap phmap.o strmap.o

phmap.o: benchs/phmap.cc
	$(CXX) -c $(CXXFLAGS) -o phmap.o -I. -I./benchs/parallel_hashmap benchs/phmap.cc

bench: bench.o strmap.o
	$(CXX) $(CXXFLAGS) -o bench bench.o strmap.o

bench.o: benchs/bench.cc
	$(CXX) -c $(CXXFLAGS) -o bench.o -I. benchs/bench.cc

words: words.o strmap.o
	$(CXX) $(CXXFLAGS) -o words words.o strmap.o

words.o: benchs/words.cc
	$(CXX) -c $(CXXFLAGS) -o words.o -I. benchs/words.cc

strmap.o: strmap.c strmap.h
	$(CC) -O2 -c $(CXXFLAGS) -o strmap.o strmap.c

clean:
	rm *.o *.exe

