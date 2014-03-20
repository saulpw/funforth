
CFLAGS= -ggdb 

ifdef DEBUG
CFLAGS+= -DDEBUG
else
CFLAGS+= -O3
endif

all: test funforth.s

funforth: funforth.c
	gcc $(CFLAGS) $< -o $@

test: funforth
	./funforth

%.s: %.c
	gcc $(CFLAGS) -S $< -o $@

clean:
	rm -f funforth funforth.s
