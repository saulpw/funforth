
CFLAGS= -ggdb 
#CFLAGS= -O3

ifdef DEBUG
CFLAGS+= -DDEBUG
endif

all: test funforth.s

funforth: funforth.c
	gcc $(CFLAGS) $< -o $@

test: funforth
	./funforth

%.s: %.c
	gcc $(CFLAGS) -S $< -o $@

clean:
	rm funforth funforth.s
