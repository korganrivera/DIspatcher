# Makefile for struct_test4 if I decide to make it a multifile project.

output: motherforking_server2.o serialise.o
	gcc motherforking_server2.o serialise.o -o output

motherforking_server2.o: motherforking_server2.c ../../Include/Serialise/serialise.h ../../Include/booking_structs.h
	gcc -c motherforking_server2.c
    
serialise.o: ../../Include/Serialise/serialise.c ../../Include/booking_structs.h
	gcc -c ../../Include/Serialise/serialise.c

clean:
	rm *.o