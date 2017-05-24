/*
** motherforking_client2.c -- Sending booking test.

    NEXT: Figure out if clients also need to be servers,
    How do I arrange that? Sometimes the server will
    need to connect to the client to send it a job.
    
    Anyway, this client will be the dispatch client. Not 
    the driver's client so concentrate on that first.

    NEXT: I want to give hostname and port, and get a sockfd 
    back. I don't want a mess of code just to get my sockfd.
    
    DONE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>         // close(), fork()
#include <errno.h>          // perror()
#include <string.h>         // memset()
#include <netdb.h>          // getaddrinfo(), struct addrinfo, gai_strerror(), freeaddrinfo()
//#include <sys/types.h>    // Why is this here?
//#include <netinet/in.h>
//#include <sys/socket.h>
//#include <arpa/inet.h>    // INET6_ADDRSTRLEN. I probably won't use this.
#include <inttypes.h>
#include "../../Include/booking_structs.h"
#include "../../Include/Serialise/serialise.h"

#define PORT "3490"         // Server's port.
#define MAXDATASIZE 100     // Max number of bytes we can get at once. Later this will be 
                            // size of my biggest packet. Ring buffer will be twice this.

                            
int   get_sockfd(char* hostname, char* port);                            
void* get_in_addr(struct sockaddr *sa);                            

                            
int main(int argc, char *argv[]){
    
    // Variables for socket work.
    int sockfd, numbytes;
    char buf[MAXDATASIZE];

    // Variables for packet work.
    uint8_t *data = NULL;
    struct _a booking = {1, 9000, 127};		// Initialise a struct to work with.


    capsule_a(&data, &booking);             // Serialise and encapsulate it.
    puts("\nCapsule ready.");
   
    
    // Check arguments.
    if (argc != 2) {
        fprintf(stderr,"usage: %s <hostname>\n", argv[0]);
        exit(1);
    }
    
    // Get a socket descriptor.
    if((sockfd = get_sockfd(argv[1], PORT)) < 0){
        if(sockfd == -1) puts("\nIs hostname legit?");
        else if(sockfd == -2) puts("\nServer is down.");
        exit(1);      
    }
    
    // Try to receive something of size MAXDATASIZE.
    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
    buf[numbytes] = '\0';

    printf("client: received '%s'\n", buf);

    // This is where I will send my booking packet.
    if (send(sockfd, data, 8, 0) == -1)
        perror("send");
    else printf("\nPacket sent!");



    free(data);
    close(sockfd);
    return 0;
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


// get sockaddr, IPv4 or IPv6:
void* get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) 
        return &(((struct sockaddr_in*)sa)->sin_addr);
    
    else
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
