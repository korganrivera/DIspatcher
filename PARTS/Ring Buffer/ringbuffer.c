/*
    Let's say I'm sending packets of data that are 100 bytes long.
    When I send them, 1-100 bytes will actually be sent.
    Part of what I receive might be the front end of the next packet in the queue.
    Data that is received will be put into a back buffer until a full packet is detected.
    When that happens, the complete packet will be moved to another buffer,
    and the content of the back buffer, if any, will be shifted to the front.
    A better solution though, is to use a ring back buffer, so only some pointers shift
    instead of copying data from one end of the buffer to the front.
    
    This might be a multithreaded process, but for this test, I want to just use a single-threaded loop.
    
    This file is just a test.    
*/

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SENT 4

int main(void){
    int8_t *buffer, *head, *tail, *start, *end;
    char sent[SENT + 1];
    
    buffer = malloc(SENT * 2 + 2);
    head = tail = start = buffer;
    end = start + SENT * 2 - 1;
    
    
    while(1){
      
        printf("\nEnter something (0-100 chars): ");
        fgets(sent, sizeof(sent), stdin);
        
        // Get rid of newline if there is one.
        size_t ln = strlen(sent);
        if(ln == 0) continue;
        else if (sent[ln - 1] == '\n')
            sent[ln - 1] = '\0';
        
        printf("\nYou entered \'%s\'", sent);
        
        // Put sent into the buffer.
        int8_t i = 0;
        int8_t str_length = strlen(sent);
        if(str_length == 0) continue;
        while(i < str_length) {
            *tail = sent[i];
            i++;
            if(tail == end) tail = start;
            else tail++;
        }

        // Check if it's complete.
        if(tail > head){
            if(tail - head >= SENT){
                i = 0;
                printf("\ncomplete packet: ");
                while(i < SENT){
                    if(*head == '\n') putchar('N');
                    else putchar(*head);
                    head++;
                    i++;
                }
            }
        }
        else if(head > tail){
            if(head - tail <= SENT){
                i = 0;
                printf("\ncomplete packet: ");
                while(i < SENT){
                    if(*head == '\n') putchar('N');
                    else putchar(*head);

                    if(head == end) head = start;
                    else head++;
                    i++;
                }
            }
        }
    }
}