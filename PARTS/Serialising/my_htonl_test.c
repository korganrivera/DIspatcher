/*
    Two functions I wrote that convert 64-bit values
    to big endian format.    2017.3.21 Korgan Rivera
*/


#include <inttypes.h>
#include <stdio.h>


uint64_t hton64(uint64_t value){
#if BYTE_ORDER == LITTLE_ENDIAN
    uint64_t value2 = 0, constant = 0xFF00000000000000;
    uint8_t i, shift;

    for(i = 0; i < 4; i++){
        shift = 56 - 16 * i;    // Gives me the sequence 56, 40, 24, 8.
        value2 |= (value & constant) >> shift;
        constant = constant >> 8;
    }

    for(i = 0; i < 4; i++){
        shift = 16 * i + 8;     // Gives me the sequence 8, 24, 40, 56.
        value2 |= (value & constant) << shift;
        constant = constant >> 8;
    }

    return value2;
#else
    return value;
#endif
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


int main(void){
    uint64_t n = 0xefcdab9078563412;

    printf("Little-endian: 0x%" PRIx64, n);
    printf("\nBig-endian:    0x%" PRIx64, hton64(n));

    puts("\n\n2nd function test:");
    printf("Little-endian: 0x%" PRIx64, n);
    printf("\nBig-endian:    0x%" PRIx64, hton64b(n));
}


