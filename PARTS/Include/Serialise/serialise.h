/*
    serialise.h

    This header requires inttypes.h and my booking_structs.h.   
*/

#ifndef _INTYPES_H
#include <inttypes.h>
#endif

#ifndef _BOOKING_STRUCTS_H
#include "../booking_structs.h"
#endif


#ifndef SERIALISE_H
#define SERIALISE_H

    void serialise_a(uint8_t *data, struct _a *booking);
    void deserialise_a(uint8_t *data, struct _a *booking);
    void serialise_b(uint8_t *data, struct _b *booking);
    void deserialise_b(uint8_t *data, struct _b *booking);
    void serialise_any(uint8_t *data, void *booking);
    void deserialise_any(uint8_t *data, void *booking);
    void capsule_a(uint8_t **data, struct _a *booking);
    void capsule_b(uint8_t **data, struct _b *booking);
    uint16_t capsule_booking(uint8_t **data, struct _booking *booking);
    void decapsule_booking(uint8_t* data, struct _booking *b);
    void print_blob(uint8_t *blob, uint16_t size);

#endif