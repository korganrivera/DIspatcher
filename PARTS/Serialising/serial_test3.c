/*
    This code is based on serial_test2.c

    Practice serialising two different types of
    struct for the taxi program. Both structs will
    begin win an id that will identify their type.
    Each piece will be put into a byte string.
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


int main(void){

    // This will hold the byte string.
    uint8_t *data;
    
    struct _a booking = {1, 9000, 127};		// Initialise a struct to work with.
	puts("First booking:\n--------------"); // Check it.
    print_structa($booking);
    data = malloc(sizeof(booking));			// Make space. Ignore packing for now.
	serialise_any(data, &booking);			// Serialise struct, put into data.
	booking = (struct _a){0};				// Wipe the struct with a compound literal.
    deserialise_a(data, &booking);
    puts("\nstruct after deserialising: "); // Check again.
    print_structa($booking);
    free(data);

    struct _b booking2 = {2, 6553};         // Initialise a second struct to work with.
    puts("\n\nSecond booking:\n---------------");
	print_structb(&booking2);
    data = malloc(sizeof(booking2));        // reuse data for second struct.
    serialise_any(data, &booking2);         // Serialise struct, put into data.
    booking2 = (struct _b){0};              // Wipe the struct with a compound literal.
    deserialise_b(data, &booking2);
    puts("\nstruct after deserialising: "); // Check again.
	print_structb(&booking2);
    free(data);
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
}


void deserialise_b(uint8_t *data, struct _b *booking){

    uint8_t *p8 = data;             // Point to first byte of data.
    booking->id = *p8; p8++;        // Copy id into booking.
    int16_t *p16 = (int16_t*)p8;    // Point to next 2 bytes of data.
    booking->nut = *p16;            // Copy c into booking.
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
    printf("booking2.id  = %" PRIu8, booking2->id);
    printf("\nbooking2.nut = %" PRId16, booking2->nut);
}