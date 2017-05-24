/*
    Practice serialising two different types of
    struct for the taxi program. Both structs will
    begin win an id that will identify their type.
    Each piece will be put into a byte string.

    NOTE: Computer uses little-endian! Not
    a problem, but don't forget this.

    NOTE: stdint.h contains the basic types.
          inttypes.h contains stdint.h and also
          stuff to use with printf, etc.

    NOTE: Not putting the larger struct elements first
    is really going to fuck with the packet size. :/
    Slop!

    Code works first time. No idea how.
    Next step: make more serialise functions for different structs and then make
    a generic umbrella function for serialising structs based on their type.
*/

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

struct _a {
    int8_t  id;
    int32_t i;
    int8_t  c;
};

void serialise_a(int8_t *data, struct _a *booking);
void deserialise_a(int8_t *data, struct _a *booking);

int main(void){

    // This will hold the byte string.
    int8_t *data;

    // Initialise a struct to work with.
    struct _a booking = {1, 9000, 127};

    // Check it.
    printf("booking.id = %d", booking.id);
    printf("\nbooking.i = %d", booking.i);
    printf("\nbooking.c = %d", booking.c);
    // printf("\n%" PRId8, booking.c);

    // Malloc enough space to hold it. Includes
    //struct packing, which I'm ignoring.
    data = malloc(sizeof(booking));

    printf("\nsizeof booking=%u", sizeof(booking));

    serialise_a(data, &booking);
    // data should contain content of the struct.

    // Let's try printing it crudely.
    puts("\nPrinting 6 bytes:");
    int8_t *q = data;
    for(int i = 0; i< 6; i++){
        printf("%u ", *q); q++;
    }

    // Wipe the struct with a compound literal.
    booking = (struct _a){0};

    // Check it.
    puts("\nstruct after clearing: ");
    printf("booking.id = %d", booking.id);
    printf("\nbooking.i = %d", booking.i);
    printf("\nbooking.c = %d", booking.c);

    deserialise_a(data, &booking);

    // Reprint struct to see if it's as it was in the beginning.
    puts("\nstruct after deserialising: ");
    printf("booking.id = %d", booking.id);
    printf("\nbooking.i = %d", booking.i);
    printf("\nbooking.c = %d", booking.c);

    free(data);
}


void serialise_a(int8_t *data, struct _a *booking){
    // Point a byte pointer to the buffer.
    int8_t *p8 = data;

    // Put the id into data, and move p8 to position after that.
    *p8 = booking->id;    p8++;

    // Point a 4-byte pointer to next position.
    int32_t *p32 = (int32_t*)p8;

    // Put i into data, and move p32 to position after that.
    *p32 = booking->i;     p32++;

    // Reuse p8 because it's a byte pointer.
    p8 = (int8_t*)p32;

    // Put c into data.
    *p8 = booking->c;

}

void deserialise_a(int8_t *data, struct _a *booking){

    int8_t *q = data;
    booking->id = *q; q++;
    int32_t *p = (int32_t*)q;
    booking->i = *p;  p++;
    q = (int8_t*)p;
    booking->c = *q;
}


