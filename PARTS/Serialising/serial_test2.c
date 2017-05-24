/*
    This code is based on serial_test1.c

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

serial_test2.c notes begin here........
    Next step: make more serialise functions for different structs and then make
    a generic umbrella function for serialising structs based on their type.

    Second test. Includes a different struct.
    RESULT: Works.

    Third test: have a generic serialising function that can take any type
    of struct, and forward it to the matching serialising function.
    RESULT: Works. :)

    NOTE: There are two things I'd like to do. First, can I pack my structs better while still having that initial id, so I can avoid slop?
    Second, Can my structs be more like objects, in that they contain their own serialising functions?

    And I have a concern: do I have to convert the variables to big-endian format before serialising them?
    When does that happen?

    Make the order of structs elements in increasing order of size. Should be the smallest way to do it.

*/

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

// This is required. It's like the 'parent form' of all other structs.
struct _base {
    uint8_t id;
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

int main(void){

    // This will hold the byte string.
    uint8_t *data;

    // Initialise a struct to work with.
    struct _a booking = {1, 9000, 127};

    // Check it.
    puts("First booking:\n--------------");
    printf("booking.id = %" PRIu8,  booking.id);
    printf("\nbooking.i = %" PRId32, booking.i);
    printf("\nbooking.c = %" PRId8, booking.c);

    // Malloc enough space to hold it. Includes
    //struct packing, which I'm ignoring for now.
    data = malloc(sizeof(booking));

    // Serialise struct, put into data.
    serialise_any(data, &booking);
    //serialise_a(data, &booking);

    /*
    // Let's try printing it crudely.
    puts("\n\nPrinting 6 bytes:");
    uint8_t *q = data;
    for(int i = 0; i< 6; i++){
        printf("%u ", *q);
        q++;
    }
    */

    // Wipe the struct with a compound literal.
    booking = (struct _a){0};

    /*
    // Check it.
    puts("\n\nstruct after clearing: ");
    printf("booking.id = %" PRIu8,  booking.id);
    printf("\nbooking.i = %" PRId32, booking.i);
    printf("\nbooking.c = %" PRId8, booking.c);
    */

    deserialise_a(data, &booking);

    // Reprint struct to see if it's as it was in the beginning.
    puts("\n\nstruct serialised.");
    puts("\nstruct after deserialising: ");
    printf("booking.id = %" PRIu8,  booking.id);
    printf("\nbooking.i = %" PRId32, booking.i);
    printf("\nbooking.c = %" PRId8, booking.c);

    free(data);


//------THE SECOND STRUCT------

    // Initialise a second struct to work with.
    struct _b booking2 = {2, 6553};

    puts("\n\nSecond booking:\n---------------");

    printf("booking2.id  = %" PRIu8, booking2.id);
    printf("\nbooking2.nut = %" PRId16, booking2.nut);

    // reuse data for second struct.
    data = malloc(sizeof(booking2));

    // Serialise struct, put into data.
    serialise_any(data, &booking2);
    //serialise_b(data, &booking2);
    // data should contain content of the struct.

    /*
    // Let's try printing it crudely.
    puts("\n\nPrinting 3 bytes:");
    q = data;
    for(int i = 0; i < 3; i++){
        printf("%" PRIu8 " ", *q);
        q++;
    }
    */

    // Wipe the struct with a compound literal.
    booking2 = (struct _b){0};

    /*
    // Check it.
    puts("\n\nstruct after clearing: ");
    printf("booking2.id  = %" PRIu8, booking2.id);
    printf("\nbooking2.nut = %" PRId16, booking2.nut);
    */

    deserialise_b(data, &booking2);

    // Reprint struct to see if it's as it was in the beginning.
    puts("\n\nstruct serialised.");
    puts("\nstruct after deserialising: ");
    printf("booking2.id  = %" PRIu8, booking2.id);
    printf("\nbooking2.nut = %" PRId16, booking2.nut);

    free(data);
}


void serialise_a(uint8_t *data, struct _a *booking){

    uint8_t *p8 = data;            // Point a byte pointer to the buffer.
    *p8 = booking->id;     p8++;   // Put the id into data, and move p8 to position after that.
    int32_t *p32 = (int32_t*)p8;   // Point a 4-byte pointer to next position.
    *p32 = booking->i;     p32++;  // Put i into data, and move p32 to position after that.
    p8 = (uint8_t*)p32;            // Reuse p8 because it's a byte pointer.
    *p8 = booking->c;              // Put c into data.

}


void deserialise_a(uint8_t *data, struct _a *booking){

    uint8_t *q = data;
    booking->id = *q; q++;
    int32_t *p = (int32_t*)q;
    booking->i = *p;  p++;
    q = (uint8_t*)p;
    booking->c = *q;
}


void serialise_b(uint8_t *data, struct _b *booking){

    uint8_t *p8 = data;          // Point a byte pointer to the buffer.
    *p8 = booking->id;    p8++;  // Put the id into data, and move p8 to position after that.
    int16_t *p16 = (int16_t*)p8; // Point a 2-byte pointer to next position.
    *p16 = booking->nut;         // Put nut into data.
}


void deserialise_b(uint8_t *data, struct _b *booking){

    uint8_t *p8 = data;
    booking->id = *p8; p8++;
    int16_t *p16 = (int16_t*)p8;
    booking->nut = *p16;
}


void serialise_any(uint8_t *data, void *booking){

    struct _base *p = (struct _base*)booking;

    printf("\nSwitch id is %" PRIu8, p->id);
    switch(p->id){
        case 1:  serialise_a(data, booking);
                 break;
        case 2:  serialise_b(data, booking);
                 break;
        default: puts("\nI ain't serialising shit.");
                 exit(1);
    }
}






