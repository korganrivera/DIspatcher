# Makefile for struct_test4 if I decide to make it a multifile project.

output: motherforking_client3.o serialise.o stringwork.o
	gcc motherforking_client3.o serialise.o stringwork.o -o output

motherforking_client3.o: motherforking_client3.c ../../Include/Serialise/serialise.h ../../Include/booking_structs.h
	gcc -c motherforking_client3.c
    
serialise.o: ../../Include/Serialise/serialise.c ../../Include/booking_structs.h
	gcc -c ../../Include/Serialise/serialise.c

stringwork.o: ../../Include/String_handling/stringwork.c
	gcc -c ../../Include/String_handling/stringwork.c

clean:
	rm *.o