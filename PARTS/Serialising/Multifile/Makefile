# Makefile for struct_test4 if I decide to make it a multifile project.

output: serial_test4.o serialise.o
	gcc serial_test4.o serialise.o -o output

serial_test4.o: serial_test4.c ../../Serialise/serialise.h ../../booking_structs.h
	gcc -c serial_test4.c
    
serialise.o: ../../Serialise/serialise.c ../../booking_structs.h
	gcc -c ../../Serialise/serialise.c

clean:
	rm *.o