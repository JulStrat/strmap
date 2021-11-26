CC = g++
#CFLAGS = -m32 -O2 -w -Wall -Wextra -Wconversion -pedantic
CFLAGS = -O2 -w -Wall -Wextra -Wconversion -pedantic

all: sm_bench sm_words

sm_bench: sm_bench.o strmap.o
	$(CC) $(CFLAGS) -o sm_bench sm_bench.o strmap.o

sm_bench.o: tests/sm_bench.cc
	$(CC) -c $(CFLAGS) -o sm_bench.o -I. tests/sm_bench.cc

sm_words: sm_words.o strmap.o
	$(CC) $(CFLAGS) -o sm_words sm_words.o strmap.o

sm_words.o: tests/sm_words.cc
	$(CC) -c $(CFLAGS) -o sm_words.o -I. tests/sm_words.cc

strmap.o: strmap.c strmap.h
	$(CC) -c $(CFLAGS) -o strmap.o strmap.c

clean:
	rm *.o *.exe

