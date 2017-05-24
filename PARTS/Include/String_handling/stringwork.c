/*
    Blockt2.c
    Turning my blockt code into a
    function so that any program
    can use it. Hows about that?
*/


/*
    What it does:
    This version is basically a word wrap program.

    Say I have a big string to print
    out. I don't want it to go all the
    way to the edge of the command line
    screen nor do I want it to be cut
    mid-word. I want it to be displayed
    with the least amount of gap on the
    right edge of an imaginary box as
    possible. So I'm going to do that.

    Original program: 2015.6.3.18:14
    This version: 2017.04.05.13:58
    Korgan Rivera
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <inttypes.h>
#include "stringwork.h"



// struct used by blockt().
typedef struct _list{
    unsigned length;
    char *p;
    struct _list *next;
}list;



int blockt(char* string, unsigned w){
/* 
 Calculates the most efficient word-wrap to apply to the 
 given string up to a maximum width of w. Then prints it.
*/
    unsigned word_count, char_count, max_width = 80;        // 80 is the default width.
    unsigned sz, longest_word, i, j, wordscout, error;
    unsigned width_limit, smallesterror, bestwidth;
    int remaining, newline_flag;
    list *curr, *root = NULL;
    char ch, *str = string;
    FILE *fp;
    
    // If w makes sense, use it.
    if(w > 0 && w <= 500) max_width = w;

    /* Confirm that there's at
   least one non-whitespace
   character. Else, quit. */
    i = 0;
    while(str[i] && isspace(str[i])) i++;
    if(!str[i]){
        puts("\nblockt(): String contains only whitespace.");
        return -1;
    }

    // Get string length.
    sz = 0;
    while(str[sz]) sz++;
	if(sz == 0) { puts("\nblockt(): empty string."); return -1; }
    
    // Build a linked list of the words in the buffer.
    i = longest_word = word_count = char_count = 0;
    do{
        // Skip over any leading whitespace that might be there.
        while(str[i] && isspace(str[i])) i++;

        // Count length of word that begins at str[i].
        for(wordscout = i; str[wordscout] && !isspace(str[wordscout]); wordscout++, char_count++);

        if(wordscout - i == 0) break;

        // If this is first word, malloc space to root.
        if(root == NULL){
            if((root = malloc(sizeof(list))) == NULL){
                puts("\nmalloc() failed. (2)"); // Fails here. (?)
                exit(1);
            }
            curr = root;

        }

        // If not first word, malloc space to curr->next.
        else{
            if((curr->next = malloc(sizeof(list))) == NULL){
                printf("\nmalloc() failed. (3)");
                exit(1);
            }
            curr = curr->next;
        }

        // Malloc space in current struct for string.
        curr->length = wordscout - i;
        if(longest_word < curr->length) longest_word = curr->length;

        if((curr->p = malloc((curr->length + 1))) == NULL){
            printf("\nmalloc() failed. (4)");
            exit(1);
        }

        // Read string into current struct.
        j = 0;
        while(i < wordscout) curr->p[j++] = str[i++];
        curr->p[j] = '\0';

        // Increment word count.
        word_count++;

    }while(str[wordscout]);
    curr->next = NULL; // Terminate linked list.
    
    // Here I need to reset sz because right now the count includes any whitespace
    // in the file. If there had been a bunch of whitespace, then sz will be off.
    sz = char_count + (word_count - 1);
    
    /* Stage 2: figure out how well the string
       would fit into a box of width i. */

    /* The largest possible error is the length
       of the string. Error will never be this. */
    smallesterror = sz;
    
    /* Start with a box that is the width of the longest word. Work up to a
       box with a width one character less than the string's total length.
       You'll never use a width equal to the string's total length, or you
       wouldn't be using this program in the first place.

       Note: the output might look screwy if the width
       of the box is wider your current screen, but it
       will look fine once you paste it into something
       with no word wrap. I might want to make this a
       user option later. */

    bestwidth = 0;

    // width_limit is the smallest of the two values, sz and MAX_WIDTH.
    width_limit = (sz <= max_width) ? sz: max_width;

    for(i = longest_word; i < width_limit ; i++){

        curr = root;
        remaining = i;
        error = 0;
        newline_flag = 1;

        while(curr){

            // Imagine you just put in the current word.
            // Calculate remaining space on line.

            /* This counts the space left on the line if you
               had just put down the first word on the line. */
            if(newline_flag){
                remaining -= curr->length;
                newline_flag = 0;
            }

            /* This counts the space left on the line if the word
               you just put down wasn't the first on the line. */
            else remaining -= (1 + curr->length);

            // If there's no next word, then this is the last line.
            // Each gap earns one point of error.
            if(!curr->next) error += remaining;
            //(!curr->next) error_calc(remaining);

            // Else, if there's not enough space left for next word,
            // calculate error for remaining gaps.
            else if(remaining < (1 + curr->next->length)){

                //error += remaining;
                if(remaining != 0) 
                    error += ((remaining * remaining + 3 * remaining) / 2);
                
                remaining = i;
                newline_flag = 1;
            }

            /* If there's no next word, then we're done. Move to
               the next size of box. Otherwise, move to next word. */
            curr = curr->next;
        }

        // Record error and width if the error is the smallest found so far.
        if(error < smallesterror){
            smallesterror = error;
            bestwidth = i;
        }

        // If solution found with no error, quit, can't beat that.
        if(!smallesterror) break;
    }

    /* No solution will be found if
       the longest word is larger
       than the max possible width. */
    if(bestwidth == 0){
        puts("\nblockt(): No solutions: widest possible block is shorter than your longest word.");
        return -1;
    }


    // Stage 3: displaying the best text block.
    curr = root;
    remaining = bestwidth;
    newline_flag = 1;
    while(curr){
            // Display first word because that will always fit.
            printf("%s", curr->p);

            // If there's no next word, then we're done.
            if(!curr->next) break;

            // Calculate remaining space on line.
            if(newline_flag){
                remaining -= curr->length;
                newline_flag = 0;
            }

            else remaining -= (1 + curr->length);

            if(remaining < (1 + curr->next->length)){
                putchar('\n');
                remaining = bestwidth;
                newline_flag = 1;
            }

            // Otherwise, if there's room for the next word, print a space.
            else putchar(' ');

            curr = curr->next;
    }
    putchar('\n');

    //Free linked list memory here. ***
    list *tmp;
    curr = root;
    while (curr != NULL){
        tmp = curr;
        curr = curr->next;
        free(tmp);
    }  

    return bestwidth;    
}



char* hex_to_char(uint8_t n){
// Takes a byte value, returns that value as a string.
// e.g. F5 -> "F5"

    char *hex_to_char[256] = {
        "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0A", "0B",
        "0C", "0D", "0E", "0F", "10", "11", "12", "13", "14", "15", "16", "17", 
        "18", "19", "1A", "1B", "1C", "1D", "1E", "1F", "20", "21", "22", "23", 
        "24", "25", "26", "27", "28", "29", "2A", "2B", "2C", "2D", "2E", "2F", 
        "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3A", "3B", 
        "3C", "3D", "3E", "3F", "40", "41", "42", "43", "44", "45", "46", "47", 
        "48", "49", "4A", "4B", "4C", "4D", "4E", "4F", "50", "51", "52", "53", 
        "54", "55", "56", "57", "58", "59", "5A", "5B", "5C", "5D", "5E", "5F", 
        "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6A", "6B", 
        "6C", "6D", "6E", "6F", "70", "71", "72", "73", "74", "75", "76", "77", 
        "78", "79", "7A", "7B", "7C", "7D", "7E", "7F", "80", "81", "82", "83", 
        "84", "85", "86", "87", "88", "89", "8A", "8B", "8C", "8D", "8E", "8F", 
        "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9A", "9B", 
        "9C", "9D", "9E", "9F", "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", 
        "A8", "A9", "AA", "AB", "AC", "AD", "AE", "AF", "B0", "B1", "B2", "B3", 
        "B4", "B5", "B6", "B7", "B8", "B9", "BA", "BB", "BC", "BD", "BE", "BF", 
        "C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "CA", "CB", 
        "CC", "CD", "CE", "CF", "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", 
        "D8", "D9", "DA", "DB", "DC", "DD", "DE", "DF", "E0", "E1", "E2", "E3", 
        "E4", "E5", "E6", "E7", "E8", "E9", "EA", "EB", "EC", "ED", "EE", "EF", 
        "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "FA", "FB", 
        "FC", "FD", "FE", "FF"
    };
    
    return hex_to_char[n];
} 



void hex_string(char *str, uint8_t *data, uint32_t length){
// Takes a data blob, converts it to a string of hex values.
// Assumes str already exists and is large enough.

    uint8_t *p8;
    uint16_t j, *p16 = (uint16_t *)str;

    // Quick check to see that str exists at least;
    if(str == NULL) return;
    
    //Builds a whole string of the hex substrings.
    for(j = 0; j < length; j++){
        strcpy((char *)p16, hex_to_char(data[j]));
        p16++;
        *p16 = ' ';
        p8 = (uint8_t*)p16;
        p8++;
        p16 = (uint16_t*)p8;
    }
}



void print_hexstr_to_block(char* str){
/* Given a string of hex values, this function
   prints them in a blockt format. */

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