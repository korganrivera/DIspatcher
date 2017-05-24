/*
    Given a blob, turn it into a string of hex characters. Given
    that, print them as a square block approximately. Instead of
    using the square root, use a rough approximation function.
*/

#include <string.h>
#include <inttypes.h>
#include <stdio.h>

void print_hexstr_to_block(char* str);

int main(void){
    char *str = "01 A2 00 01 01 A2 00 33 12"; 

    hexstr_to_block(str);    
}

/* Takes a string that looks like 
   "A0 B3 F2" 
   and prints it in a block shape.
*/
void print_hexstr_to_block(char* str){
    uint16_t sq_root, i, count, str_length = strlen(str);
    uint16_t blocks = (str_length + 1) / 3;
    float x = blocks;
    
    // Limit block to 80 chars wide.
    // If only a few blocks, don't even bother.
    if(blocks <= 8 || blocks > 729) sq_root = 27;
    else{
        
        // Good for square roots of 1 <= x <= 729
        float fsq_root = 5.457E-13 * (x * x * x * x * x) - 1.166E-9
                         * (x * x * x * x) + 9.554E-7 * (x * x * x)
                         - 3.893E-4 * (x * x) + 0.108 * x + 2.314;
            
        // Round up.
        sq_root = (unsigned)(fsq_root + 0.5);
    }
        
    // Print the fucking thing.
    count = i = 0;
    while(i < str_length){
        putchar(*(str + i));
        i++;
        putchar(*(str + i));
        i += 2;
        count++;
        if(count == sq_root){
            count = 0;
            putchar('\n');
        }
        else
            putchar(' ');
    }    
}


