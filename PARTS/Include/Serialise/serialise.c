/*
    serialise.c

    Contains functions that serialise
    and/or encapsulate structs types
    defined in booking_structs.h

    Currently working on capsule_booking().
    DONE. 
    
    Ignoring that long/lat exists for now. Need to 
    learn how to pack floats. 2017.4.1
    When I use float types for long/lat, I will only 
    need 32-bit numbers.
 */

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

#include "../../Include/booking_structs.h"
#include "serialise.h"  // Header for this file.

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

#if BYTE_ORDER == LITTLE_ENDIAN         // If host machine is little-endian, convert
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


// This serialises and encapsulates struct _a types.
// First 2 bytes are the size.
void capsule_a(uint8_t **data, struct _a *booking){

    uint16_t data_length = 6;               // struct_a is 6 bytes.
    *data = malloc(2 + data_length);        // 2 bytes to tell the size.

    uint16_t *p16 = (uint16_t *)(*data);    // Point to first 2 bytes.
    *p16 = data_length;                     // Put data length into data.  
       
#if BYTE_ORDER == LITTLE_ENDIAN             // If host machine is little-endian, convert 
    *p16 = htons(*p16);                     // this 2-byte value to big-endian.
#endif

    p16++;                                  // Move forward.
    uint8_t *p8 = (uint8_t*)p16;            // Point to next byte.
    *p8 = booking->id; p8++;                // Put struct type in there. Move forward.
    int32_t *p32 = (int32_t*)p8;            // Point to next 4 bytes.
    *p32 = booking->i;                      // Copy i into data.

#if BYTE_ORDER == LITTLE_ENDIAN             // If host machine is little-endian, convert 
    *p32 = htonl(*p32);                     // this 4-byte value to big-endian.
#endif

    p32++;                                  // Move forward.
    p8 = (uint8_t*)p32;                     // Point to next byte.
    *p8 = booking->c;                       // Copy c into data.
}


// This serialises and encapsulates struct _b types.
// First 2 bytes are the size.
void capsule_b(uint8_t **data, struct _b *booking){

    uint16_t data_length = 3;               // struct_b is 3 bytes.
    *data = malloc(2 + data_length);        // 2 bytes to tell the size.

    uint16_t *p16 = (uint16_t *)(*data);    // Point to first 2 bytes.
    *p16 = data_length;                     // Put data length into data.  
       
#if BYTE_ORDER == LITTLE_ENDIAN             // If host machine is little-endian, convert 
    *p16 = htons(*p16);                     // this 2-byte value to big-endian.
#endif

    p16++;                                  // Move forward.
    uint8_t *p8 = (uint8_t*)p16;            // Point to next byte.
    *p8 = booking->id; p8++;                // Put struct type in there. Move forward.
    p16 = (int16_t*)p8;                     // Point to next 2 bytes.
    *p16 = booking->nut;                    // Copy i into data.

#if BYTE_ORDER == LITTLE_ENDIAN             // If host machine is little-endian, convert 
    *p16 = htonl(*p16);                     // this 4-byte value to big-endian.
#endif
}



// This serialises and encapsulates struct 
// _booking types. First 2 bytes are the size.
uint16_t capsule_booking(uint8_t **data, struct _booking *booking){
    // data size does not include the size variable itself.

    // The size part is going to be ugly...
    uint16_t data_length = 1 + 4 + strlen(booking->location.start) +
                           strlen(booking->location.end) + strlen(booking->name) +
                           strlen(booking->phone) + strlen(booking->account_number)
                           + strlen(booking->email) + strlen(booking->info) +
                           strlen(booking->time) + strlen(booking->flight_num)
                           + 9;
                           
    *data = malloc(2 + data_length);        // 2 bytes to tell the size.

    uint16_t *p16 = (uint16_t *)(*data);    // Point to first 2 bytes.
    *p16 = data_length;                     // Put data length into data.  
       
#if BYTE_ORDER == LITTLE_ENDIAN             // If host machine is little-endian, convert 
    *p16 = htons(*p16);                     // this 2-byte value to big-endian.
#endif

    p16++;                                  // Move forward.
    uint8_t *p8 = (uint8_t*)p16;            // Point to next byte.
    *p8 = booking->struct_id; p8++;         // Put struct type in there. Move forward.
    int32_t *p32 = (int32_t*)p8;            // Point to next 4 bytes. (job_id)
    *p32 = booking->job_id;                 // Copy job_id into data.

#if BYTE_ORDER == LITTLE_ENDIAN             // If host machine is little-endian, convert 
    *p32 = htonl(*p32);                     // this 4-byte value to big-endian.
#endif

    p32++;                                  // Move forward.
    p8 = (uint8_t*)p32;                     // Point to next byte. location start string.

    strcpy(p8, booking->location.start);    // This probably works. Copy string into data.
    p8 += 1 + strlen(booking->location.start);  // Move forward.
    
    strcpy(p8, booking->location.end);      
    p8 += 1 + strlen(booking->location.end);

    strcpy(p8, booking->name);              
    p8 += 1 + strlen(booking->name);        
    
    strcpy(p8, booking->phone);             
    p8 += 1 + strlen(booking->phone);       
    
    strcpy(p8, booking->account_number);       
    p8 += 1 + strlen(booking->account_number); 

    strcpy(p8, booking->email);              
    p8 += 1 + strlen(booking->email);        

    strcpy(p8, booking->info);              
    p8 += 1 + strlen(booking->info);        
    
    strcpy(p8, booking->time);              
    p8 += 1 + strlen(booking->time);        
    
    strcpy(p8, booking->flight_num);        
    p8 += 1 + strlen(booking->flight_num);        

    // If everything worked, it should look something like this:
    // 030|3|0|Here\0|There\0|Jim\0|555\0|9\0|a@b.com\0|Fish\0|ASAP\0|911\0
    
    return data_length + 2;
}



void print_blob(uint8_t *blob, uint16_t size){
    uint16_t i;
    
    printf("\n0x");
    for(i = 0; i < size; i++)
        printf("%" PRIx8, blob[i]);
}

/* struct _location{
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
 */
 
 
 
 
 
 // This only works when the booking already exists in main().
// I still have to malloc the char pointers though.
// This version only for testing.
void decapsule_booking(uint8_t* data, struct _booking *b){
    uint16_t *p16 = (uint16_t *)data;
    p16++;                          // Skip over first 2 bytes: size.
    uint8_t *p8 = (uint8_t *)p16;   // Point to struct_id. 
    b->struct_id = *p8;             // Copy struct_id.
    p8++;                           // Skip over 1 byte.
    uint32_t *p32 = (uint32_t *)p8; // Point to next 4 bytes: job_id;
    b->job_id = *p32;               // Copy job_id.
    
#if BYTE_ORDER == LITTLE_ENDIAN     // If host machine is little-endian, convert
    b->job_id = ntohl(b->job_id);   // this 4-byte value to little-endian.
#endif
    
    p32++;                          // Skip over 4 bytes.
    p8 = (uint8_t *)p32;            // Point to next byte: Start of 'start'.
    
    uint16_t string_length = strlen(p8);            // Get length of string without null.
    b->location.start = malloc(string_length + 1);  // Make space for it.
    strcpy(b->location.start, p8);                  // Copy string into b inc. null.
    p8 += string_length + 1;                        // Move to next string: 'end'.

    string_length = strlen(p8);                     // Get length of string without null.
    b->location.end = malloc(string_length + 1);    // Make space for it.
    strcpy(b->location.end, p8);                    // Copy string into b inc. null.
    p8 += string_length + 1;                        // Move to next string: 'name'.
    
    string_length = strlen(p8);                     // Get length of string without null.
    b->name = malloc(string_length + 1);            // Make space for it.
    strcpy(b->name, p8);                            // Copy string into b inc. null.
    p8 += string_length + 1;                        // Move to next string: 'phone'.
    
    string_length = strlen(p8);                     // Get length of string without null.
    b->phone = malloc(string_length + 1);           // Make space for it.
    strcpy(b->phone, p8);                           // Copy string into b inc. null.
    p8 += string_length + 1;                        // Move to next string: 'account_number'.

    string_length = strlen(p8);                     // Get length of string without null.
    b->account_number = malloc(string_length + 1);  // Make space for it.
    strcpy(b->account_number, p8);            // Copy string into b inc. null.
    p8 += string_length + 1;                        // Move to next string: 'email'.
    
    string_length = strlen(p8);                     // Get length of string without null.
    b->email = malloc(string_length + 1);           // Make space for it.
    strcpy(b->email, p8);                     // Copy string into b inc. null.
    p8 += string_length + 1;                        // Move to next string: 'info'.
    
    string_length = strlen(p8);                     // Get length of string without null.
    b->info = malloc(string_length + 1);            // Make space for it.
    strcpy(b->info, p8);                      // Copy string into b inc. null.
    p8 += string_length + 1;                        // Move to next string: 'time'.

    string_length = strlen(p8);                     // Get length of string without null.
    b->time = malloc(string_length + 1);            // Make space for it.
    strcpy(b->time, p8);                      // Copy string into b inc. null.
    p8 += string_length + 1;                        // Move to next string: 'flight_num'.

    string_length = strlen(p8);                     // Get length of string without null.
    b->flight_num = malloc(string_length + 1);      // Make space for it.
    strcpy(b->flight_num, p8);                // Copy string into b inc. null.   
}

 