
CFLAGS= -ggdb 

ifdef DEBUG
CFLAGS += -DDEBUG
else
CFLAGS += -O3
endif

all: test funforth.s

funforth: funforth.o main.o | words.inc
	gcc $^ -o $@

test: funforth
	./funforth

%.s: %.c
	gcc -O3 -S $< -o $@

clean:
	rm -f funforth funforth.s *.o
