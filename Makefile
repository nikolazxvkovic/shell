all: clean shell

shell:
	gcc -std=c99 -Wall -pedantic -g -D_POSIX_C_SOURCE=200809L main.c scanner.c pipeLine.c shell.c -o shell

clean:
	rm -f *~
	rm -f *.o
	rm -f shell
