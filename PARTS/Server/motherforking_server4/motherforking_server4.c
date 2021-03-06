/*
**  motherforking_server4.c

    Copy of moth...3.c: This version is me trying out
    my new recv_packet function.
    
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
#include "../../Include/booking_structs.h"
#include "../../Include/Serialise/serialise.h"
#include "../../Include/String_handling/stringwork.h"
 
#define PORT "3490" // the port users will be connecting to
#define BACKLOG 10 // how many pending connections queue will hold
#define MAXDATASIZE 1024
 
// If I still want to using processes, I need to use shared memory for this!
static uint32_t *job_id;                                // A counter to number jobs as they are 
                                                        // added. This will be server's job but 
                                                        // for testing, I'm using this.

 
 
void print_booking(struct _booking *b);
int recv_packet(int sockfd, uint8_t **buff);
 
void sigchld_handler(int s){
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}
 
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
 
int main(void)
{
    int sockfd, new_fd;                     // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;    
    struct sockaddr_storage their_addr;     // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    uint8_t buf[MAXDATASIZE];
    int rv;
    int numbytes;
    struct _booking booking; 
     
    // Variables for packet work.
    uint8_t *data = NULL;
        
    // Setup shared mem for job_id.
    job_id = mmap(NULL, sizeof *job_id, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);    
    *job_id = 1; // Much later, this will be loaded from a file.
        
    // Set hints.
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;     // use my IP
 
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
    sa.sa_handler = sigchld_handler; // reap all dead processes
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
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("\n\nserver: got connection from %s\n", s);
        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
             
            int packet_size;
            if((packet_size = recv_packet(new_fd, &data)) < 1){
                if(packet_size == 0){
                    fprintf(stderr, "\nrecv_packet(): data length zero. Is that cool?");
                }
                else if(packet_size == -1)
                    perror("recv_packet");
                else if(packet_size == -2){
                    fprintf(stderr, "\nrecv_packet(): malloc fail");
                }
            }
            
            else printf("\npacket received!\nPacket size = %d bytes\n", packet_size);
            
            //print_blob((uint8_t *)data, (uint16_t)(packet_size));
            hex_string(buf, (uint8_t *)data, packet_size);
            print_hexstr_to_block(buf);
            
            // Wipe booking struct.
            booking = (struct _booking){0};
            
            // Deserialise the data from where it already is into a booking.
            decapsule_booking(data, &booking);
            
            // Give the booking a job_id.
            booking.job_id = *job_id;
            *job_id = *job_id + 1;
            // printf("\njob_id value is now %" PRIu32, *job_id);
            
            // Send back the job_id as confirmation.
            int n;
            uint32_t id = booking.job_id;

            printf("\nSending job_id = %" PRIu32, id);
            
            #if BYTE_ORDER == LITTLE_ENDIAN     // If host machine is little-endian, convert
            id = htonl(id);                     // this 4-byte value to little-endian.
            #endif
           
            if((n = send(new_fd, &id, 4, 0)) == -1){
                perror("send");
                puts("\nI couldn't send id for some reason...");
                break;
            }
            
            else puts("\n\nRecipt sent!");


            
            // Wait for client to close.
            while(recv(new_fd, &job_id, 4, 0) != 0);
            puts("\nConnection closed.\n\nReady.");

            // Booking should now contain a struct.
            print_booking(&booking);
            
            return 0;
        }
        else close(new_fd); // parent doesn't need this
    }
    munmap(job_id, sizeof *job_id);
    return 0;
}


void print_booking(struct _booking *b){
    puts("\n\nBOOKING\n-------");
    printf("job id: %" PRIu32 "\n", b->job_id);
    printf("\nstart: %s", b->location.start);
    printf("\nend: %s", b->location.end);
    printf("\nname: %s", b->name);
    printf("\nphone: %s", b->phone);
    printf("\nemail: %s", b->email);
    printf("\ninfo: %s", b->info);
    printf("\ntime: %s", b->time); 
    printf("\nflight #: %s", b->flight_num);
}




int recv_packet(int sockfd, uint8_t **buff){
/* Receives any sized packet on sockfd, given that the packet
is prefixed with 2 bytes that tell the size of the remaining
packet. The size will not include these 2 bytes. */    
    uint16_t data_length, copy, *p16;
    uint16_t n, bytesleft = 2, total = 0;
    uint8_t *p8 = (uint8_t *)&data_length;
    
    // Read 2 bytes from socket into data_length.
    while(bytesleft){
        if((n = recv(sockfd, p8 + total, bytesleft, 0)) == -1){
            perror("recv1");
            break;
        }
        else if(n == 0) break;                  // Client closed.
        else{
            total += n;
            bytesleft -= n;
        }
    }

    if(n > 0){                                  // If successful, get rest of packet.
        copy = data_length;                     // I need a host order copy of data_length.
        #if BYTE_ORDER == LITTLE_ENDIAN
        copy = ntohs(copy);
        #endif        
        
        copy += 2;                              // Need 2 more bytes to hold the size field.

        if((*buff = malloc(copy)) == NULL){     // Malloc space to hold entire packet.
            puts("my_recv: malloc() fail");
            return -2;                          // ...or whatever error code is appropriate.
        }  

        p16 = (uint16_t*)*buff;                 // Point to start of buffer.
        *p16 = data_length;                     // put data_length in it, the network byte order version.
        p16++;                                  // Move forward 2 bytes.
        p8 = (uint8_t*)p16;                     // Point to 3rd byte of buffer.
        
        total = 0;                              // Read <copy> bytes from socket into buffer.
        bytesleft = copy;
        while(bytesleft){
            if((n = recv(sockfd, p8 + total, bytesleft, 0)) == -1){
                perror("recv2");
                break;
            }
            else if(n == 0) break;              // Client closed.
            else{
                total += n;
                bytesleft -= n;
            }
        }
        
        if(n > 0)                               // I have received the complete packet!
            return copy;                        // Return size of the entire packet.
    }

    else return n;                              // If any recv() failed, return its error code.
}