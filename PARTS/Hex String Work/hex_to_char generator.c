/*
    Generates the character equivelant of hex values.
    e.g. 0f becomes  '0''F'
    Fills an array with values from 0 to FF.
    This is just a test to see if I can do it.
*/
#include <inttypes.h>
#include <stdio.h>

int main(void){
    uint8_t n;
    uint8_t nibble;
    char hex_to_char[256][2], flag = 0;
        
    
    // This only works if box is little-endian.
    n = 0;
    do{
        if(n == 255) flag = 1;
        nibble = (n & 240) >> 4;
        if(nibble >= 0 && nibble <= 9) 
            hex_to_char[n][0] = nibble + 48;
        else if(nibble >= 10 && nibble <= 15)
            hex_to_char[n][0] = nibble + 55;


        nibble = n & 15;
        if(nibble >= 0 && nibble <= 9)
            hex_to_char[n][1] = nibble + 48;

        else if(nibble >= 10 && nibble <= 15)
            hex_to_char[n][1] = nibble + 55;
        
        //printf("\n%" PRIx16, n);
        printf("\"%c%c\", ", hex_to_char[n][0], hex_to_char[n][1]);
        n++;
    }while(!flag);
}