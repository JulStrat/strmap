CC  = gcc
CXX = c++
#CXXFLAGS = -m32 -O2 -w -Wall -Wextra -Wconversion -Wshadow -pedantic
CXXFLAGS = -O2 -w -Wall -Wextra -Wconversion -Wshadow -pedantic

all: bench words hashmap

bench: bench.o strmap.o
	$(CXX) $(CXXFLAGS) -o bench bench.o strmap.o

bench.o: benchs/bench.cc
	$(CXX) -c $(CXXFLAGS) -o bench.o -I. benchs/bench.cc

hashmap: hashmap.o strmap.o
	$(CXX) $(CXXFLAGS) -o hashmap hashmap.o strmap.o

hashmap.o: benchs/hashmap.cc
	$(CXX) -c $(CXXFLAGS) -o hashmap.o -I. benchs/hashmap.cc

words: words.o strmap.o
	$(CXX) $(CXXFLAGS) -o words words.o strmap.o

words.o: benchs/words.cc
	$(CXX) -c $(CXXFLAGS) -o words.o -I. benchs/words.cc

strmap.o: strmap.c strmap.h
	$(CC) -std=c89 -ansi -c $(CXXFLAGS) -o strmap.o strmap.c

clean:
	rm *.o *.exe

