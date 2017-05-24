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
*/


#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>  // htonl

struct _base {          // This will be used as though it were
    uint8_t id;         // the base class of all other structs.
};                      

struct _a {
    uint8_t  id;
    int32_t i;
    int8_t  c;
};

struct _b {
    uint8_t  id;
    int16_t  nut;
};


void serialise_a(uint8_t *data, struct _a *booking);
void deserialise_a(uint8_t *data, struct _a *booking);
void serialise_b(uint8_t *data, struct _b *booking);
void deserialise_b(uint8_t *data, struct _b *booking);
void serialise_any(uint8_t *data, void *booking);
void deserialise_any(uint8_t *data, void *booking);
void print_structa(struct _a *booking);
void print_structb(struct _b *booking);
void print_blob(uint8_t *blob, uint16_t size);

int main(void){

    // This will hold the byte string.
    uint8_t *data;
    
    struct _a booking = {1, 9000, 127};		// Initialise a struct to work with.
	puts("First booking:\n--------------"); // Check it.
    print_structa(&booking);
    data = malloc(6);			            // Make space for data only.
	serialise_any(data, &booking);			// Serialise struct, put into data.
	booking = (struct _a){0};				// Wipe the struct with a compound literal.
    deserialise_a(data, &booking);
    puts("\nstruct after deserialising: "); // Check again.
    print_structa(&booking);
    free(data);

    struct _b booking2 = {2, 6553};         // Initialise a second struct to work with.
    puts("\n\nSecond booking:\n---------------");
	print_structb(&booking2);
    data = malloc(3);                      // reuse data for second struct.
    serialise_any(data, &booking2);         // Serialise struct, put into data.
    booking2 = (struct _b){0};              // Wipe the struct with a compound literal.
    deserialise_b(data, &booking2);
    puts("\nstruct after deserialising: "); // Check again.
	print_structb(&booking2);
    //free(data);

    // Testing out how to encapulate a packet. 
    // I'll test this on data from booking2.
    uint16_t data_length = 3;                   // Don't like this constant. Solve.
    uint8_t *capsule = malloc(3 + data_length); // 2 bytes to tell the size, 1 byte to tell the type of struct.
    uint16_t *p1 = (uint16_t*)capsule;          // Point to first 2 bytes.
    *p1 = data_length; p1++;                    // Put size of data into capsule.
    uint8_t *p2 = (uint8_t*)p1;                 // Point to next byte.
    *p2 = booking2.id; p2++;                    // Put struct type in there.
    for(uint16_t i = 0; i < data_length; i++){  // Copy data into capsule.
        *p2 = data[i];
        p2++;
    }
    
    // capsule now has all the things.
    puts("\n\nPrinting the 2nd data blob and its capsule.");
    // Print capsule, just to check it out.
    print_blob(data, data_length);
    print_blob(capsule, 3 + data_length);
    
    free(data);
    free(capsule);
    
}


void serialise_a(uint8_t *data, struct _a *booking){

    uint8_t *p8 = data;             // Point to first byte of data.
    *p8 = booking->id;              // Copy id into data.
    p8++;                           // Move p8 to next position.
    int32_t *p32 = (int32_t*)p8;    // Point to next 4 bytes.
    *p32 = booking->i;              // Copy i into data.

#if BYTE_ORDER == LITTLE_ENDIAN     // If host machine is little-endian, convert 
    *p32 = htonl(*p32);             // this 4-byte value to big-endian.
#endif

    p32++;
    p8 = (uint8_t*)p32;             // Point to next byte.
    *p8 = booking->c;               // Copy c into data.
}


void deserialise_a(uint8_t *data, struct _a *booking){

    uint8_t *q = data;              // Point to first byte of data.
    booking->id = *q; q++;          // Copy id into booking.
    int32_t *p = (int32_t*)q;       // Point to next 4 bytes.
    booking->i = *p;  p++;          // Copy i into booking.

#if BYTE_ORDER == LITTLE_ENDIAN     // If host machine is little-endian, convert
    booking->i = ntohl(booking->i); // this 4-byte value to little-endian.
#endif

    q = (uint8_t*)p;                // Point to next byte of data.
    booking->c = *q;                // Copy c into booking.
}


void serialise_b(uint8_t *data, struct _b *booking){

    uint8_t *p8 = data;             // Point to first byte of data.
    *p8 = booking->id;    p8++;     // Copy id into data.
    int16_t *p16 = (int16_t*)p8;    // Point to next 2 bytes.
    *p16 = booking->nut;            // Copy nut into data.
    
#if BYTE_ORDER == LITTLE_ENDIAN     // If host machine is little-endian, convert 
    *p16 = htons(*p16);             // this 2-byte value to big-endian.
#endif    
}


void deserialise_b(uint8_t *data, struct _b *booking){

    uint8_t *p8 = data;             // Point to first byte of data.
    booking->id = *p8; p8++;        // Copy id into booking.
    int16_t *p16 = (int16_t*)p8;    // Point to next 2 bytes of data.
    booking->nut = *p16;            // Copy c into booking.

#if BYTE_ORDER == LITTLE_ENDIAN     // If host machine is little-endian, convert
    booking->nut = ntohs(booking->nut); // this 2-byte value to little-endian.
#endif
}


void serialise_any(uint8_t *data, void *booking){

    struct _base *p = (struct _base*)booking;

    switch(p->id){
        case 1:  serialise_a(data, booking);
                 break;
        case 2:  serialise_b(data, booking);
                 break;
        default: puts("\nI ain't serialising shit.");
                 exit(1);
    }
}


void deserialise_any(uint8_t *data, void *booking){

    struct _base *p = (struct _base*)booking;

    switch(p->id){
        case 1:  deserialise_a(data, booking);
                 break;
        case 2:  deserialise_b(data, booking);
                 break;
        default: puts("\nI ain't deserialising shit.");
                 exit(1);
    }
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