# Makefile for motherforking_client2.c

output: motherforking_client2.o serialise.o
	gcc motherforking_client2.o serialise.o -o output

motherforking_client2.o: motherforking_client2.c ../../Include/Serialise/serialise.h ../../Include/booking_structs.h
	gcc -c motherforking_client2.c
    
serialise.o: ../../Include/Serialise/serialise.c ../../Include/booking_structs.h
	gcc -c ../../Include/Serialise/serialise.c

clean:
	rm *.o