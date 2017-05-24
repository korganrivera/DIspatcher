/*
    My attempt to bundle up all the socket stuff, to
    make them more manageable, useful and readable.
    This is the client part.
    This is the linux version.
    Lots of mistakes to fix and error checking to include.

    Remember to zero out the buffer and sin_zero later on.
    Maybe replace gethostbyname with getaddrinfo or getnameinfo when I know more.
*/

#include <stdio.h>
#include <stdlib.h>        // exit()
#include <unistd.h>
#include <string.h>        // memset(), memcpy()
#include <netdb.h>         // h_errno, herror(), gethostbyname()
// Compiles without the following two header files. How? :/
//#include <sys/socket.h>  // struct sockaddr, socklen_t.
//#include <netinet/in.h>  // struct sockaddr_in

int main(int argc, char *argv[]) {
    int sockfd;
    int portno = 5152;
    char buffer[256];
    char *server_ip = "localhost";
    char *message = "Hello.";
    struct sockaddr_in serv_addr;
    struct hostent *server;

    // Get socket descriptor.
    printf("Setting up socket...");
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        herror("socket() error");
        exit(1);
    }

    // Fill hostent struct.
    // Replace gethostbyname() with getaddrinfo() later.
    printf("\nFilling hostent struct...");
    if((server = gethostbyname(server_ip)) == NULL){
        herror("gethostbyname() error");
        exit(1);
    }
    printf("done.");

    // Wipe serv_addr.
    printf("\nWiping serv_addr...");
    memset((struct sockaddr_in*) &serv_addr, 0, sizeof(serv_addr));
    printf("...done.");

    // Fill serv_addr.
    printf("\nFilling serv_addr...");
    serv_addr.sin_family = AF_INET;
    memcpy(&(serv_addr.sin_addr.s_addr), &(server->h_addr), sizeof(server->h_addr));
    serv_addr.sin_port = htons(portno);
    printf("done.");

    // Code lags here. No idea why.

    // The following three blocks work even
    // when the server isn't running. HOW?!

    // Connect the socket.
    printf("\nConnecting...");
    connect(sockfd, (struct sockaddr *) &(serv_addr), sizeof(serv_addr));
    printf("done.");

    // Send and receive a string.
    printf("\nSending string...");
    write(sockfd, message, sizeof(message));
    printf("done.");

    printf("\nReading string...");
    read(sockfd, buffer, sizeof(buffer));
    printf("done.");

    // Code stops here.

    printf("client says: %s\nserver says: %s", message, buffer);
    printf("\nClosing socket...");
    close(sockfd);
    printf("done.");
}

