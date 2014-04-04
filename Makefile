
CFLAGS= -ggdb -m32

ifdef DEBUG
CFLAGS += -DDEBUG
else
CFLAGS += -O3
endif

all: test funforth.s

funforth.o: funforth.h words.inc compiler.inc
main.o: funforth.h

funforth: funforth.o main.o
	gcc $(CFLAGS) $^ -o $@

test: funforth
	./funforth

%.s: %.c
	gcc -O3 -S $< -o $@

clean:
	rm -f funforth funforth.s *.o
