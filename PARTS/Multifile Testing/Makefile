# Makefile for multifile test.

output: main.o a.o b.o
	gcc main.o a.o b.o -o output

main.o: main.c structs.h
	gcc -c main.c
    
a.o: a.c structs.h
	gcc -c a.c
    
b.o: b.c structs.h
	gcc -c b.c

clean:
	rm *.o