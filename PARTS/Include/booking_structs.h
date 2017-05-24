/*
    booking_structs.h
    
    Contains the structs that describe bookings.
    Requires stdint.h.
        
*/
#ifndef _STDINT_H
#include <stdint.h>
#endif

#ifndef BOOKING_STRUCTS_H
#define BOOKING_STRUCTS_H

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

    struct _location{
        char *start;                // 140 bytes.
        char *end;                  // 140 bytes.
    };

    struct _booking {
        uint8_t struct_id;          // Use id 3 for now.
        uint32_t job_id;            // Let server set this. Running count.
        struct _location location;
        
        // All these strings will have length limits.
        // While experimenting, assume the following.
        char *name;                 // 206. Longest name in the US :)
        char *phone;                // 20
        char *account_number;       // 4 bytes. Why not.
        char *email;                // 254 bytes max. Max length of a possible email.
        char *info;                 // 500 bytes max. Why not.
        char *time;                 // use struct tm later. Use 140 bytes for now.
        char *flight_num;           // XY9999. 8 bytes max.
        struct _booking *next;
    };

#endif