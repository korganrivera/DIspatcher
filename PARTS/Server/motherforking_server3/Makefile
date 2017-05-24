# Makefile for motherforking_server3.c

output: motherforking_server3.o serialise.o
	gcc motherforking_server3.o serialise.o -o output

motherforking_server3.o: motherforking_server3.c ../../Include/Serialise/serialise.h ../../Include/booking_structs.h
	gcc -c motherforking_server3.c
    
serialise.o: ../../Include/Serialise/serialise.c ../../Include/booking_structs.h
	gcc -c ../../Include/Serialise/serialise.c

clean:
	rm *.o