/*
    struct size test.
    rearrange elements in a struct to see which will have the
    least padding. Why? I'd like to pack my structs better,
    even when they have a mandatory byte at the start.
    RESULTS
    .-----------------------------,
    | order   | sizeof(struct _a) |
    |-----------------------------|
    | a,b,c,d | 16                |
    | a,b,d,c | 24                |
    | a,c,b,d | 24                |
    | a,c,d,b | 24                |
    | a,d,b,c | 24                |
    | a,d,c,b | 24                |
    | b,a,c,d | 24                |
    | b,a,d,c | 24                |
    | b,c,a,d | 24                |
    | b,c,d,a | 24                |
    | b,d,a,c | 24                |
    | b,d,c,a | 24                |
    | c,a,b,d | 24                |
    | c,a,d,b | 32                |
    | c,b,a,d | 24                |
    | c,b,d,a | 32                |
    | c,d,a,b | 24                |
    | c,d,b,a | 24                |
    | d,a,b,c | 24                |
    | d,a,c,b | 32                |
    | d,b,a,c | 24                |
    | d,b,c,a | 32                |
    | d,c,a,b | 24                |
    | d,c,b,a | 24                |
    '-----------------------------'
*/

#include <stdio.h>
#include <inttypes.h>

struct _a {
    uint8_t   byte;
    uint8_t   a;
    uint16_t  b;
    uint32_t  c;
    uint64_t  d;
};

int main(void){

    printf("size: %u", sizeof(struct _a));
}
