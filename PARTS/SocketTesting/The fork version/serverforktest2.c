/*
   Going to turn this forking 
   server into my other server, 
   so I can use it.

   Think this is making zombies. fix that.
   
   */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <signal.h>

#define PORT "6666"
#define BACKLOG 10

static uint32_t *job_id;

void hex_string(char *str, uint8_t *data, uint32_t length);
void print_hexstr_to_block(char* str);
char* hex_to_char(uint8_t n);

void sigchld_handler(int s){
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

 /*  TO DO LIST                     | DONE
        ---------------------------------------
        Sync with clients.          |  
                                    |  
                                    |  
                                    |  
                                    |  
                                    |  
                                    |  
    
    */



int main(void){

    struct addrinfo hints, *servinfo, *p;    
    int rv, sockfd, new_fd, yes = 1;
    struct sigaction sa;
    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    
    // Set hints.
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;     // use my IP
 
    // Set shared memory for job_id.
    job_id = mmap(NULL, sizeof *job_id, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *job_id = 999;
 
    // Get servinfo linked list.
    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
 
    // Loop through all the results and bind to the first we can.
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }
    freeaddrinfo(servinfo); // all done with this structure
 
    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    // Zombie killer
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }




    printf("server: waiting for connections...\n");
    while(1) { // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        
        // Fork this shit up.
        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
        
            // First thing to do is to receipt the front two bytes of the packet.
            uint32_t new_id;
            uint16_t packetsize = 2;         // Length of string including terminator.
            uint16_t totalsent = 0;
            uint16_t bytesleft = packetsize; 
            uint16_t buffer16, buffer16host;
            uint16_t *p16;
            uint8_t *p8 = (uint8_t *)&buffer16;
            uint8_t *packet, *hexpacket;
            int n;
            
            // Get two bytes, read them into buffer16.
            while(totalsent < packetsize){
                n = recv(new_fd, p8 + totalsent, bytesleft, 0);
                if(n == -1){
                    perror("recv1");
                    break;
                }
        
                totalsent += n;
                bytesleft -= n;
            }

            if(n == -1){
                puts("\nError receiving packet.");
            }
            else{
                printf("\n2 bytes received successfully");
                
                // Convert 2 bytes to a host 
                // order so I can print it.
                #if BYTE_ORDER == LITTLE_ENDIAN
                buffer16host = ntohs(buffer16);
                #endif            
                
                printf("\nThe 2-byte value is %" PRIu16, buffer16host);
                
                // Now I need to malloc space for the rest 
                // of the packet, plus the first two bytes.
                packet = malloc(buffer16host + 2);
                
                // Put first two bytes into the packet.
                p16 = (uint16_t *)packet;
                *p16 = buffer16;
                p16++;
                p8 = (uint8_t *)p16;
                // Read rest of packet.
                // Get two bytes, read them into buffer16.
                packetsize = buffer16host;
                totalsent = 0;
                bytesleft = packetsize;
                while(totalsent < packetsize){
                    n = recv(new_fd, p8 + totalsent, bytesleft, 0);
                    if(n == -1){
                        perror("recv1");
                        break;
                    }
            
                    totalsent += n;
                    bytesleft -= n;
                }
             
                if(n == -1){
                    puts("\nError receiving packet.");
                }
                
                else{
                    printf("\nRest of packet received.");
                    new_id = *job_id;
                    (*job_id)++;
                    
                    // Send back new_id like a receipt.
                     #if BYTE_ORDER == LITTLE_ENDIAN
                    new_id = htonl(new_id);
                    #endif    
                    
                    packetsize = 4;         // Length of string including terminator.
                    totalsent = 0;
                    bytesleft = packetsize; 
                  
                    while(totalsent < packetsize){
                        n = send(new_fd, &new_id + totalsent, bytesleft, 0);
                        if(n == -1){
                            perror("send1");
                            break;
                        }
                    
                        totalsent += n;
                        bytesleft -= n;
                    }

                    if(n == -1){
                        puts("\nError sending receipt.");    
                    }
                
                    else{
                        puts("\nreceipt sent successfully.");
                    }
                  
                    // Print it to check. Malloc space for the 
                    // hex version.
                    hexpacket = malloc(buffer16host + 2);
                    hex_string(hexpacket, (uint8_t *)packet, buffer16host + 2);
                    print_hexstr_to_block(hexpacket);
                
                    /* At this point, I can do something with the packet.
                        The first thing I want to do is check if the packet 
                        is a sync request. If the struct_id == 4, then I need to 
                        send all the packets in my list to the client.
                    */
                
                    // Quick check.
                    p16 = (uint16_t *)packet;
                    p16++;
                    p8 = (uint8_t *)p16;
                    if(*p8 == 4) puts("\nThis is a sync request.");
                    else puts("\nThis is not a sync request.");
                }
            }

            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    }
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
 