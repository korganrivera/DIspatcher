# Makefile for motherforking_server4.c

output: motherforking_server4.o serialise.o stringwork.o
	gcc motherforking_server4.o serialise.o stringwork.o -o output

motherforking_server4.o: motherforking_server4.c ../../Include/Serialise/serialise.h ../../Include/booking_structs.h ../../Include/String_handling/stringwork.h
	gcc -c motherforking_server4.c
    
serialise.o: ../../Include/Serialise/serialise.c ../../Include/booking_structs.h
	gcc -c ../../Include/Serialise/serialise.c

stringwork.o: ../../Include/String_handling/stringwork.c
	gcc -c ../../Include/String_handling/stringwork.c
clean:
	rm *.o