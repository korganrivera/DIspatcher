/*
    motherforking_client3.c
    This is a mash-up of motherforking_client2.c and basic_dispatch_menu.
    
    Currently working on capsule_booking(). Need to test it and
    also deal with float values of long/lat in the packet. Also,
    still need to somehow attach instructions to the packet.
    
    -----
    
    I need to attach an instruction to the booking so that the server knows
    what to do with it. Consider using an enum as an instruction list. e.g.
    enum {ADD, DEL, EDIT, GIVE, TAKE}
    
    ADD  - add this booking to the list.
    DEL  - delete this booking from the list. (just need job id.)
    EDIT - Find the booking that matches this booking's id, update the differences.
    GIVE - Give this booking to this driver. (just need job id and driver id.)
    TAKE - Take this booking from this driver. (just need job id and driver id.)
    
    So I'll also need to include job id and driver id.
    
    ALSO, I'll need to sync with server. So I'll need the client to do a
    sync request when it starts up and the server will send the dispatch
    an entire list. So it'll send a list of packets, and the dispatch
    will receive each one and make a list of them. Or, the server can
    send a big blob of structs and the dispatcher can split them and so
    on. Whichever is faster. Probably the second option. Maybe. For now,
    let's just see if I can send this new booking struct.
*/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

#include "../../Include/booking_structs.h"
#include "../../Include/Serialise/serialise.h"
#include "../../Include/String_handling/stringwork.h"
#define MAXDATASIZE 1024

#define PORT "3490"
#define HOST "localhost"


void show_menu(void);
int get_choice(void);                                   // Return menu choice: 1-6 only.
void make_booking(struct _booking** curr);              // Create and append booking to list.
void show_list(struct _booking* curr);                  // Display list.
void delete(struct _booking** rt);                      // Delete booking in list.
uint16_t fgets_no_newline(char *str, uint16_t size);    // Get string, strip newline, return length.
void get_string(char **str, uint16_t size);             // Get string, malloc for it, put it in str.
int get_sockfd(char* hostname, char* port);

uint32_t job_id = 0;                                    // A counter to number jobs as they are 
                                                        // added. This will be server's job but 
                                                        // for testing, I'm using this.
                        
int main(void){

    // Variables for socket work.
    int sockfd, numbytes;
    int8_t buf[MAXDATASIZE];

    // Variables for packet work.
    uint8_t *data = NULL;
    
    // Variables for bookings.
    int ch;
    struct _booking *curr, *root;
    
    curr = root = NULL;
    
    // printf("\e[H\e[2J"); // Clear screen with magic.
    
    do{
        show_menu();
        ch = get_choice();
        
        if(ch == 1){
            // ADD A BOOKING.
            // If linked list is empty, create booking 
            // on root. Else, make it on curr.
            if(root == NULL) {
                make_booking(&root);
                curr = root;
            }
            else {
                make_booking(&(curr->next));
                curr = curr->next;
            }
            // Send booking to the server.
            puts("\nMaking capsule...");
            uint16_t packet_size = capsule_booking(&data, curr);             // Serialise and encapsulate it.
            puts("...capsule ready.");
            printf("\npacket_size = %" PRIu16 " bytes\n", packet_size);
            //print_blob(data, packet_size);
            hex_string(buf, (uint8_t *)data, packet_size);
            print_hexstr_to_block(buf);
            
            // Get a socket descriptor.
            if((sockfd = get_sockfd(HOST, PORT)) < 0){
                if(sockfd == -1) puts("\nIs hostname legit?");
                else if(sockfd == -2) puts("\nServer is down.");
                //exit(1);      
            }

            
            else{

                // Send my booking packet.
                puts("\n\nsending packet...");
                uint16_t total = 0;
                uint16_t bytesleft = packet_size;
                uint16_t n;

                while(total < packet_size){
                        if((n = send(sockfd, data + total, bytesleft, 0) == -1)){
                            perror("send1");
                            break;
                        }
                        if (n == -1) { break; }
                        total += n;
                        bytesleft -= n;
                }
                if(n == -1) perror("send2");
                else puts("Packet sent!");
            }
        
            // I need to add a condition here that deals with situations
            // where the packet doesn't send successfully. The booking 
            // info can remain in an 'edit space' and I can retry the send.
            // That way, when the server comes back up, I don't have to 
            // retype anything. Maybe have a buffer of unsent stuff.
            
            // Wait for confirmation. This fails. Not sure why. 
            puts("\nWaiting for receipt.");
            if ((numbytes = recv(sockfd, &job_id, 4, 0)) == -1)
                perror("recv1");
            
            else{
                uint32_t id = job_id;
                #if BYTE_ORDER == LITTLE_ENDIAN     // If host machine is little-endian, convert
                id = ntohl(id);                     // this 4-byte value to little-endian.
                #endif

                printf("\nconfirmation received! (%d bytes)", numbytes);
                printf("\njob_id received: %" PRIu32, id);
                
                // close socket.
                close(sockfd);
                break;
            }
                
    

            free(data);
            close(sockfd); // if sockfd is garbage here, this still works I think.
        }

        
        else if(ch == 2){
            delete(&root);
            
            // Point curr at the last entry.
            curr = root;
            if(curr) 
                while(curr->next) curr = curr->next;
        }

        else if(ch == 3){
            // dispatch;
        }

        else if(ch == 4){
            // edit();
        }
        
        else if(ch == 5){
            show_list(root);
        }

        else if(ch == 6){
            // QUIT. free mem here.
            puts("\n\nGoodbye.");
        }
        
    }while(ch != 6);
}



void show_menu(){

      printf("\n"
           "\n,--------------."
           "\n| BOOKING MENU |"
           "\n|--------------|"
           "\n| 1 | ADD      |"
           "\n| 2 | DELETE   |"
           "\n| 3 | DISPATCH |"
           "\n| 4 | EDIT     |"
           "\n| 5 | SHOW     |"
           "\n| 6 | QUIT     |"
           "\n'--------------'"
           "\n\n: ");    
 }



void make_booking(struct _booking** curr){
    
    uint16_t size = 1024;
    
    if(((*curr) = malloc(sizeof(struct _booking))) == NULL){
        puts("make_booking(): malloc fail.");
        exit(1);
    }
    
    (*curr)->struct_id = 3;      // Identify the struct type to rebuild it later.
    (*curr)->job_id = 666;  // Give booking a number. Server will do this later.
    
    /* Note to my future self. Get a load of this pointer work
       below! Apparently, I hate my future self, but it works so
       that's all that matters (almost). Sorry dude! 2017/3/30 */
    
    printf("\n\nlocation start: ");
    get_string(&((*curr)->location.start), size);   
    
    printf("location end: ");
    get_string(&((*curr)->location.end), size);
    
    printf("name: ");
    get_string(&((*curr)->name), size);
    
    printf("phone #: ");
    get_string(&((*curr)->phone), size);
    
    printf("acc #: ");
    get_string(&((*curr)->account_number), size);
    
    printf("email: ");
    get_string(&((*curr)->email), size);
    
    printf("info: ");
    get_string(&((*curr)->info), size);
    
    printf("time: ");
    get_string(&((*curr)->time), size);
    
    printf("flight #: ");
    get_string(&((*curr)->flight_num), size);
    
    
    (*curr)->next = NULL;
    
    printf("\nBooking created.");
}



void show_list(struct _booking* curr){

    if(!curr){
        puts("\nEmpty list.");
        return;
    }

    while(curr){
        puts("\n\nBOOKING\n-------");
        printf("job id: %" PRIu32 "\n", curr->job_id);
        printf("\nstart: %s", curr->location.start);
        printf("\nend: %s", curr->location.end);
        printf("\nname: %s", curr->name);
        printf("\nphone: %s", curr->phone);
        printf("\nemail: %s", curr->email);
        printf("\ninfo: %s", curr->info);
        printf("\ntime: %s", curr->time);
        printf("\nflight: %s", curr->flight_num);
        
        curr = curr->next;
    }
}



void delete(struct _booking** rt){
    
    uint32_t id;
    
    puts("\n\nDELETE BOOKING");
    
    // If list empty, return.
    if(*rt == NULL){
        puts("List empty. Nothing to delete.");
        return;
    }
    
    // Get id of booking to delete.
    printf("Enter id to delete: ");
    scanf("%" SCNu32, &id);
  
    struct _booking *tmp = *rt;

    // If root matches, delete it.
    if((**rt).job_id == id){
        *rt = (*tmp).next;
        free(tmp);
        puts("\nDeleted root.");
        return;
    }
    
    // Look for a non-root match, and delete it.
    while((*tmp).next){
        if((*tmp).next->job_id == id){
            struct _booking *tmp2 = (*tmp).next;
            (*tmp).next = tmp2->next;
            free(tmp2);
            puts("\nDeleted.");
            return;
        }
        else
            tmp = (*tmp).next;
    }

    puts("\nNot found.");
}





// Input: address of unallocated char pointer, number of bytes.
// Action: reads up to <size> chars from user into buffer.
// Measures length of input. Mallocs that much space for str.
// Copies buffer into str. Null terminates string.
// to-do: error check malloc, deal with str if already allocated.
void get_string(char **str, uint16_t size){
        char buffer[size];
        fgets(buffer, size, stdin);
        uint16_t i;
        
        // Measure.
        for(i = 0; buffer[i] && buffer[i] != '\n'; i++);
        uint16_t str_len = i + 1;
        
        *str = malloc(str_len);
        
        // Copy.
        for(i = 0; buffer[i] && buffer[i] != '\n'; i++)
            (*str)[i] = buffer[i];
        
        (*str)[i] = '\0';
}



// Only reads first character of user's input.
// So if you enter 123, it thinks you entered 1.
// Not a big deal, but might want to fix it.
int get_choice(void){
    int choice;

    do{
        choice = getchar();
        if(choice == '\n') break;
        choice -= 48;               // Convert char to int equiv: '3' -> 3.
        getchar();                  // Ignore newline.
    
        if(choice < 1 || choice > 6)
            printf("\n1 - 6 only: ");
    
    }while(choice < 1 || choice > 6);
    
    return choice;
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
