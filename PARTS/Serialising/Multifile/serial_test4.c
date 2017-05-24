/*
    This code is based on serial_test3.c

    Practice serialising two different types of
    struct for the taxi program. Both structs will
    begin win an id that will identify their type.
    Each piece will be put into a byte string.
    
    In this version, I need to encapsulate my
    data with size and type info, so that the
    receiver knows how much data to receive
    and what to do with it once it has it.
    
    DONE. size, id, and data are written to capsule.
    
    NEXT: All these operations included struct padding, which is unnecessary.
    Get rid of this factor. The only thing in data should be data.
    
    DONE. 
    
    NEXT: make a capsule function which serialises and then encapsulates a struct.    
    
    DONE.
    
    NEXT: Fucking send it.
    
    
*/

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>       // htonl
#include <inttypes.h>
#include "../../Include/booking_structs.h"
#include "../../Include/Serialise/serialise.h"

void print_structa(struct _a *booking);
void print_structb(struct _b *booking);
void print_blob(uint8_t *blob, uint16_t size);

int main(void){

    // This will hold the byte string.
    uint8_t *data = NULL;
    
    struct _a booking = {1, 9000, 127};		// Initialise a struct to work with.
	puts("First booking:\n--------------"); // Check it.
    print_structa(&booking);
    data = malloc(6);			            // Make space for data only.
	serialise_any(data, &booking);			// Serialise struct, put into data.
	booking = (struct _a){0};				// Wipe the struct with a compound literal.
    printf("\nPrinting data blob: ");       // Check what's in data, as hex values.
    print_blob(data, 6);
    deserialise_a(data, &booking);
    puts("\nstruct after deserialising: "); // Check again.
    print_structa(&booking);
    free(data);

    // Testing out how to encapulate a packet. 
    // This both serialises and encapsulates a packet.
    capsule_a(&data, &booking);
    
    printf("\n\nPrinting capsule: ");       // capsule now has all the things.
    print_blob(data, 8);                    // Print capsule, just to check it out.
    free(data);    

    struct _b booking2 = {2, 6553};         // Initialise a second struct to work with.
    puts("\n\nSecond booking:\n---------");
	print_structb(&booking2);
    data = malloc(3);                       // reuse data for second struct.
    serialise_any(data, &booking2);         // Serialise struct, put into data.
    booking2 = (struct _b){0};              // Wipe the struct with a compound literal.
    deserialise_b(data, &booking2);
    puts("\nstruct after deserialising: "); // Check again.
	print_structb(&booking2);
    free(data);
}


uint64_t hton64b(uint64_t x){
#if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t *s = (uint8_t *)&x;
    return (uint64_t)((uint64_t)s[0] << 56 | (uint64_t)s[1]
           << 48 | (uint64_t)s[2] << 40 | (uint64_t)s[3] <<
           32 | (uint64_t)s[4] << 24 | (uint64_t)s[5] << 16
           | (uint64_t)s[6] << 8 | (uint64_t)s[7]);
#else
    return x;
#endif
}


void print_structa(struct _a *booking){
	printf("booking.id = %" PRIu8,  booking->id);
    printf("\nbooking.i = %" PRId32, booking->i);
    printf("\nbooking.c = %" PRId8, booking->c);
}


void print_structb(struct _b *booking){
    printf("booking2.id  = %" PRIu8, booking->id);
    printf("\nbooking2.nut = %" PRId16, booking->nut);
}


void print_blob(uint8_t *blob, uint16_t size){
    uint16_t i;
    
    printf("\n0x");
    for(i = 0; i < size; i++)
        printf("%" PRIx8, blob[i]);
}