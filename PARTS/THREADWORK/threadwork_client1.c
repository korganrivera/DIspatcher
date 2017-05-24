/*
    Here's the plan. I need to practice threadwork. I
    need 2+ clients to bombard a server with input.
    The server needs to receive these inputs, and add
    them to a list. It needs to use mutexes to access
    the list concurrently. Clients might send delete
    requests too. For now, they'll just send numbers.
    
    The other major part is that the
    server must send new entries and
    deletes to all other clients so
    they can update their lists too.

*/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

#define HOST "localhost"
#define PORT "6666"


int get_sockfd(char* hostname, char* port);
void hex_string(char *str, uint8_t *data, uint32_t length);
void print_hexstr_to_block(char* str);
char* hex_to_char(uint8_t n);
int append(struct _booking **head, struct _booking **end, struct _booking booking);
void show_list(struct _booking* head);
void show_menu(void);
int get_choice(void);
void get_string(char **str, unsigned size);
void delete(struct _booking **head, struct _booking **end, uint32_t id);

void print_blob(uint8_t *blob, uint16_t size){
    uint16_t i;
    
    printf("\n0x");
    for(i = 0; i < size; i++)
        printf("%" PRIx8, blob[i]);
}


int main(void){

    uint8_t packet;
    time_t t;
    
    /* Intializes random number generator */
   srand((unsigned) time(&t));
    
    do{
    
    
        /* First thing I have to do is connect to 
           the server, and sync the jobs list. */
        // Get a socket descriptor.
        int sockfd = get_sockfd(HOST, PORT);
        if(sockfd < 0){
            puts("\nSomething is fucky. Can't\nsync. Is server down?");
        }

        else{
            // Send server a random byte number.
    
            packet = 1 + rand() % 100;  // 1 - 100
    
            // Send packet.
            uint16_t p_size = 1;
            uint16_t totalsent = 0;
            uint16_t bytesleft = p_size; 
            int n;

            // This is overkill for sending one byte, but whatevs.
            while(totalsent < p_size){
                n = send(sockfd, packet + totalsent, bytesleft, 0);
                if(n == -1){
                    perror("send1");
                    break;
                }
                totalsent += n;
                bytesleft -= n;
            }
                
            if(n == -1){
                puts("\nError sending packet.");    
            }
                    
            else{        
                puts("\npacket sent successfully.");
            }
            close(sockfd);
        }//else        
    
    }while(1);
}
            
            
            
            
            
            
            
int get_sockfd(char* hostname, char* port){
    
    struct addrinfo hints, *servinfo, *p;
    int rv, sockfd;
    //char s[INET6_ADDRSTRLEN];
    
    // Wipe and initialise hints struct.
    memset(&hints, 0, sizeof hints);    
    hints.ai_family = AF_UNSPEC;        
    hints.ai_socktype = SOCK_STREAM;
        
    // Get list of addresses associated with given IP and port.
    if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "get_sockfd: getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }
   
   // Loop through all the results and connect to the first one that I can.
    for(p = servinfo; p != NULL; p = p->ai_next) {

        // Try to establish a socket.
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("get_sockfd(): socket()");
            continue;
        }

        // Try to connect on that socket.
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("get_sockfd(): connect()");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "get_sockfd(): failed to connect\n");
        return -2;
    }
    
    // Display who I'm connected to.
    //inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    //printf("client: connected to %s\n", s);

    freeaddrinfo(servinfo); // No longer need servinfo.
    return sockfd;
}



