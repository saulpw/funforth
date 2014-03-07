
test: funforth
	./funforth

funforth: funforth.c funforth.s
	gcc -ggdb $< -o $@

%.s: %.c
	gcc -S $< -o $@

clean:
	rm funforth funforth.s
