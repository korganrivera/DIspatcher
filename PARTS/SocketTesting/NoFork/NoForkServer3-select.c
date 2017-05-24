/* 
** 

    Mashup of Beej's selectserver.c and my NoForkServer2.c.
    
   Can do what NoForkServer2.c did but with multiply clients connected at the
    same time, using select(). Still need to work out the details, and change
    the client code so that they try to remain connected all the time.

    
    After that, I need to be able to relay calls to the relevant parties, i.e.
    sending call to driver needs to relay packet to driver.
    if a dispatcher adds a call to the list, that packet needs to be relayed to all other
    connected dispatchers.
    
    I'll need a system to group these people.
    I could use the linux file permissions flag type thing.
    rwxrwxrwx. But instead just dispatch|driver
    
    so 0|1 = driver, not dispatcher.
       1|0 = dispatcher, not driver.
       1|1 = driver, dispatcher.
       0|0 = nothing.
            
    socket will be the key in the list.
    Under that will be cab number, user name, and somewhere will be that
    user's type: dispatcher, driver, both, etc. I figure that would be
    part of a database, but I could hard code it in the beginning.
    
    Korgan.
    
    Problem: crashes when I delete the root. I thought I fixed this but I guess not.
    

*/

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <netdb.h>


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


#define PORT "6666"   // port we're listening on
#define ADD_BOOKING      3
#define SYNC_REQ         4
#define DEL_BOOKING      5
#define EDIT_BOOKING     6
#define DISPATCH_BOOKING 7
#define TAKEBACK_BOOKING 8
#define ALIVE_BEACON     9

uint32_t job_id = 74656;

struct _booking {
    uint8_t struct_id;          
    uint32_t job_id;            
    char *start;                
    char *end;
    char *name;                 
    char *phone;                
    char *account_number;       
    char *email;                
    char *info;                 
    char *time;                 
    char *flight_number;        
    struct _booking *next;
    struct _booking *prev;
};

void hex_string(char *str, uint8_t *data, uint32_t length);
void print_hexstr_to_block(char* str);
char* hex_to_char(uint8_t n);
void decapsule_booking(uint8_t* data, struct _booking *b);
int append(struct _booking **head, struct _booking **end, struct _booking booking);
void show_list(struct _booking* head);
void delete_by_id(struct _booking **head, struct _booking **end, uint32_t id);
int send_loop(int sockfd, uint8_t *packet, uint16_t size);
int recv_loop(int sockfd, uint8_t *packet, uint16_t size);
uint16_t capsule_booking(uint8_t **data, struct _booking *booking);
struct _booking* find_booking(struct _booking *head, struct _booking *end, uint32_t id);
int delete_booking(struct _booking **head, struct _booking **end, struct _booking *target);



// get sockaddr, IPv4 or IPv6: 
void *get_in_addr(struct sockaddr *sa) {

    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void) {

    fd_set master;    // master file descriptor list    
    fd_set read_fds;  // temp file descriptor list for select()    
    int fdmax;        // maximum file descriptor number
    
    int listener;     // listening socket descriptor    
    int newfd;        // newly accept()ed socket descriptor    
    struct sockaddr_storage remoteaddr;  // client address    
    socklen_t addrlen;
    
    char buf[256];    // buffer for client data    
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];

    int yes = 1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

    struct addrinfo hints, *ai, *p;
    
    FD_ZERO(&master);    // clear the master and temp sets    
    FD_ZERO(&read_fds);
    
    struct _booking *curr, *root;
    struct _booking booking_buffer;
    curr = root = NULL;
    
    // get us a socket and bind it.--------------------------------------------
    memset(&hints, 0, sizeof hints); 
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }        
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
             continue;
        }                
        
        // lose the pesky "address already in use" error message        
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }
        break;
    }
    
    // if we got here, it means we didn't get bound    
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }
    
    freeaddrinfo(ai); // all done with this
    
    // Start listening.--------------------------------------------------------
    
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    // add the listener to the master set    
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor    
    fdmax = listener; // so far, it's this one
    
    // main loop.--------------------------------------------------------------   
    for(;;) {
        read_fds = master; // copy it        
        printf("\nReady. Connect to port %s to use.\n", PORT);
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }
        
        // run through the existing connections looking for data to read        
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!                
                if (i == listener) {
                    // handle new connections                    
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } 
                    else {
                        FD_SET(newfd, &master); // add to master set                        
                        if (newfd > fdmax) {    // keep track of the max                            
                            fdmax = newfd;
                        }                        
                        printf("selectserver: new connection from %s on"
                         " socket %d\n", inet_ntop(remoteaddr.ss_family,
                         get_in_addr((struct sockaddr*)&remoteaddr),
                         remoteIP, INET6_ADDRSTRLEN), newfd);
                    }
                } 
                else {
                   
                    // we got some data from a client                        
                    // Socket accepted. Get first two bytes.-------------------------------
                    uint16_t packetsize = 2;         
                    uint16_t buffer16, buffer16host;
                    uint16_t *p16;
                    uint8_t *p8 = (uint8_t *)&buffer16;
                    uint8_t *packet, *hexpacket;
                    int n;

                    printf("\npacketsize is %" PRIu16 ".", packetsize);
                    if((n = recv_loop(i, p8, packetsize)) == -1){
                            puts("\nproblem.");
                            close(i); // bye!                        
                            FD_CLR(i, &master); // remove from master set           
                    }
                    else if(n == 0){
                        // connection closed                            
                        printf("selectserver: socket %d hung up\n", i);    
                        close(i); // bye!                        
                        FD_CLR(i, &master); // remove from master set   
                    }     
                         
                         
                    else{

                        printf("\n2 bytes received successfully.");

                        buffer16host = buffer16;
                        #if BYTE_ORDER == LITTLE_ENDIAN
                        buffer16host = ntohs(buffer16host);
                        #endif            

                        buffer16host += 2;
                        packet = malloc(buffer16host);

                        p16 = (uint16_t *)packet;
                        *p16 = buffer16;
                        p16++;
                        p8 = (uint8_t *)p16;

                        packetsize = buffer16host - 2;
                        if((n = recv_loop(i, p8, packetsize)) == -1){
                            puts("\nproblem.");
                        }
                         
                        else{
                            puts("\nRest of packet received.");

                            // Print packet to check.--------------------------------------
                            hexpacket = malloc(buffer16host);
                            hex_string(hexpacket, (uint8_t *)packet, buffer16host);
                            print_hexstr_to_block(hexpacket); 

                            // Receive the rest of the packet.-----------------------------
                            p16 = (uint16_t *)packet;
                            p16++;
                            p8 = (uint8_t *)p16;
                             
                            /* If packet is a sync request, send---------------------------
                            the quantity of packets about to be
                            sent, followed by all the packets. */
                            if(*p8 == SYNC_REQ) {

                                puts("\nPacket is a sync request.");

                                struct _booking *scout = root;
                                uint16_t quantity, quantityhost = 0;
                                 
                                // Count bookings in the list.
                                while(scout){
                                    quantityhost++;
                                    scout = scout->next;                        
                                }
                                printf("\nThere are %" PRIu16 " packets to send.\n", quantityhost);

                                // Convert quantity to big-endian if necessary.
                                quantity = quantityhost;
                                #if BYTE_ORDER == LITTLE_ENDIAN
                                quantity = htons(quantity);
                                #endif

                                // Put big-endian quantity into the packet.
                                p8 = (uint8_t *)&quantity;
                                 
                                // Send quantity to client.
                                packetsize = 2;         
                                send_loop(i, p8, packetsize);
                                // Needs error check here.
                                 
                                // Send all the packets.
                                scout = root;
                                for(int j = 0; j < quantityhost; j++){                        
                                    
                                    free(packet);
                                    uint16_t packet_size = capsule_booking(&packet, scout);

                                    int n;
                                    if((n = send_loop(i, packet, packet_size)) == -1){
                                        puts("\nError sending packet.");   
                                        break;                            
                                    }

                                    else{ 
                                        printf("\nPacket %d sent.", i);
                                    }   

                                    scout = scout->next;
                                }

                                // If I sent packets, check if 
                                // the operation was successful.
                                if(quantityhost > 0){

                                    if(n == -1){
                                        puts("\nThere was a sync-send error.");                        
                                    }
                                    else{
                                        puts("\nAll sync done successfully probably.");
                                    }
                                }
                            }
                            /* If the packet was a new booking, give the booking-----------
                               a job_id, send this number back to the client, and 
                               add the new booking to the list. */
                            else if(*p8 == ADD_BOOKING){ 

                                puts("\nPacket is a new booking.");

                                uint32_t new_id = job_id;
                                uint32_t new_idhost = new_id;

                                if(job_id == 4294967295)
                                    job_id = 0;
                                else
                                    job_id++;

                                #if BYTE_ORDER == LITTLE_ENDIAN
                                new_id = htonl(new_id);
                                #endif    

                                packetsize = 4;         
                                int n;
                                uint8_t *p8 = (uint8_t *)&new_id;
                                
                                if((n = send_loop(i, p8, packetsize)) == -1){
                                    puts("\nError sending receipt.");    
                                }

                                else{
                                    puts("\nreceipt sent successfully.");

                                    booking_buffer = (struct _booking){0};

                                    decapsule_booking(packet, &booking_buffer);
                                    booking_buffer.job_id = new_idhost;

                                    if(append(&root, &curr, booking_buffer) != 1) 
                                        puts("\nappend fucked up.");
                                    else puts("\nbooking added to list.");

                                    show_list(root);
                                }         
                            }

                            // If the packet was a delete request, do it.------------------
                            else if(*p8 == DEL_BOOKING){
                                puts("\nPacket is a delete request");
                                p8++;
                                
                                uint32_t *jobid_to_delete = (uint32_t*)p8;
                                uint32_t jitd = *jobid_to_delete;                    

                                #if BYTE_ORDER == LITTLE_ENDIAN
                                jitd = ntohl(jitd);
                                #endif
                                printf("\nRequest to delete job %" PRIu32, jitd);
                                delete_by_id(&root, &curr, jitd); //problem is here.
                                show_list(root);
                            }

                            /* If the packet was an edited booking, search for the---------
                               booking and if found replace it with the new booking. */
                            else if(*p8 == EDIT_BOOKING){
                                puts("\nPacket is an edit request");

                                booking_buffer = (struct _booking){0};
                                decapsule_booking(packet, &booking_buffer);

                                struct _booking *scout = find_booking(root, curr, booking_buffer.job_id);

                                if(scout == NULL)
                                    puts("\nNot found.");

                                else{
                                    delete_booking(&root, &curr, scout);
                                    append(&root, &curr, booking_buffer);
                                    puts("\nList entry updated.");
                                    show_list(root);
                                }
                            }

                            else if(*p8 == DISPATCH_BOOKING){
                                puts("\nPacket is a dispatch request");
                            }

                            else if(*p8 == TAKEBACK_BOOKING){
                                puts("\nPacket is a takeback request");
                            }

                            else if(*p8 == ALIVE_BEACON){
                                puts("\nClient sent a STILL_ALIVE beacon.");
                            }

                            else{
                                puts("\nI don't recognise this packet type. :/");
                            }
                        }    
                    }      
            
                        // Code for sending stuff to everyone else.
                        /* for(j = 0; j <= fdmax; j++) {
                            // send to everyone!                            
                            if (FD_ISSET(j, &master)) {
                               
                               // except the listener and ourselves                                
                                if (j != listener && j != i) {
                                    if (send(j, buf, nbytes, 0) == -1) {
                                        perror("send");
                                    }                                
                                }                            
                            }
                         }// END for loop */            
                  
                } // END handle data from client            
            } // END got new incoming connection        
        } // END looping through file descriptors    
    } // END for(;;)--and you thought it would never end!        
    return 0;
 } 
 
 
 void hex_string(char *str, uint8_t *data, uint32_t length){

    uint8_t *p8;
    uint16_t j, *p16 = (uint16_t *)str;

    if(str == NULL) return;

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

    uint16_t sq_root, i, count, str_length = strlen(str);
    uint16_t blocks = (str_length + 1) / 3;
    float x = blocks;

    if(blocks <= 8 || blocks > 729) sq_root = 27;
    else{

        float fsq_root = 5.457E-13 * (x * x * x * x * x) - 1.166E-9
                         * (x * x * x * x) + 9.554E-7 * (x * x * x)
                         - 3.893E-4 * (x * x) + 0.108 * x + 2.314;

        sq_root = (unsigned)(fsq_root + 0.5);
    }

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

void decapsule_booking(uint8_t* data, struct _booking *b){
    uint16_t *p16 = (uint16_t *)data;
    p16++;                          
    uint8_t *p8 = (uint8_t *)p16;   
    b->struct_id = *p8;             
    p8++;                           
    uint32_t *p32 = (uint32_t *)p8; 
    b->job_id = *p32;               

    #if BYTE_ORDER == LITTLE_ENDIAN     
    b->job_id = ntohl(b->job_id);   
    #endif

    p32++;                          
    p8 = (uint8_t *)p32;            

    uint16_t string_length = strlen(p8);            
    b->start = malloc(string_length + 1);  
    strcpy(b->start, p8);                  
    p8 += string_length + 1;                        

    string_length = strlen(p8);                     
    b->end = malloc(string_length + 1);    
    strcpy(b->end, p8);                    
    p8 += string_length + 1;                        

    string_length = strlen(p8);                     
    b->name = malloc(string_length + 1);            
    strcpy(b->name, p8);                            
    p8 += string_length + 1;                        

    string_length = strlen(p8);                     
    b->phone = malloc(string_length + 1);           
    strcpy(b->phone, p8);                           
    p8 += string_length + 1;                        

    string_length = strlen(p8);                     
    b->account_number = malloc(string_length + 1);  
    strcpy(b->account_number, p8);            
    p8 += string_length + 1;                        

    string_length = strlen(p8);                     
    b->email = malloc(string_length + 1);           
    strcpy(b->email, p8);                     
    p8 += string_length + 1;                        

    string_length = strlen(p8);                     
    b->info = malloc(string_length + 1);            
    strcpy(b->info, p8);                      
    p8 += string_length + 1;                        

    string_length = strlen(p8);                     
    b->time = malloc(string_length + 1);            
    strcpy(b->time, p8);                      
    p8 += string_length + 1;                        

    string_length = strlen(p8);                     
    b->flight_number = malloc(string_length + 1);      
    strcpy(b->flight_number, p8);                
} 

int append(struct _booking **head, struct _booking **end, struct _booking booking){
/* Appends a booking to the end of the linked
   list. If no list exists, it initiates one.
*/

    struct _booking *current;
    struct _booking *old_end;
    
    // If root doesn't exist, make it.
    if(*head == NULL){
        if(((*head) = malloc(sizeof(struct _booking))) == NULL){
            puts("append(): malloc fail1.");
            return -1;
        }
        *end = *head;
    }
    
    // If root exists, make new node after last node.
    else{
        if((((*end)->next) = malloc(sizeof(struct _booking))) == NULL){
            puts("append(): malloc fail2.");
            return -1;
        }
        old_end = *end;
        *end = (*end)->next;
    } 
    
    // Does it even make sense to do this
    // when I haven't malloced for the strings?
    // I'm fairly certain I have to allocate each string.
        
    current = *end;
        
    (*current).struct_id = booking.struct_id;
    (*current).job_id = booking.job_id;
    
    // Make space for all the things.
    if(((*current).start = malloc(strlen(booking.start + 1))) == NULL){
         puts("append(): malloc fail 3.");
        return -1;
    }
    if(((*current).end = malloc(strlen(booking.end + 1))) == NULL){
         puts("append(): malloc fail 4.");
        return -1;
    }
    if(((*current).name = malloc(strlen(booking.name + 1))) == NULL){
         puts("append(): malloc fail 5.");
        return -1;
    }
    if(((*current).phone = malloc(strlen(booking.phone + 1))) == NULL){
         puts("append(): malloc fail 6.");
        return -1;
    }
    if(((*current).account_number = malloc(strlen(booking.account_number + 1))) == NULL){
         puts("append(): malloc fail 7.");
        return -1;
    }
    if(((*current).email = malloc(strlen(booking.email + 1))) == NULL){
         puts("append(): malloc fail 8.");
        return -1;
    }
    if(((*current).info = malloc(strlen(booking.info + 1))) == NULL){
         puts("append(): malloc fail 9.");
        return -1;
    }
    if(((*current).time = malloc(strlen(booking.time + 1))) == NULL){
         puts("append(): malloc fail 10.");
        return -1;
    }
    if(((*current).flight_number = malloc(strlen(booking.flight_number + 1))) == NULL){
         puts("append(): malloc fail 11.");
        return -1;
    }
    
    // Copy all the things. 
    strcpy((*current).start, booking.start);
    strcpy((*current).end, booking.end);
    strcpy((*current).name, booking.name);
    strcpy((*current).phone, booking.phone);
    strcpy((*current).account_number, booking.account_number);
    strcpy((*current).email, booking.email);
    strcpy((*current).info, booking.info);
    strcpy((*current).time, booking.time);
    strcpy((*current).flight_number, booking.flight_number);
    
    (*current).next = NULL;
        
    // If I just made root, prev is NULL.
    if(current == *head){
        (*current).prev = NULL;
    }

    // otherwise, set prev to the previous end.
    else
        (*current).prev = old_end;
    
    //**end = booking;
    //(*end)->next = NULL;
    
    return 1;
}



void show_list(struct _booking* head){
    unsigned length, longest, i, j;
 
    if(!head){
        puts("\nEmpty list.");
        return;
    }
 
    while(head){  
     
        // Going to print this all purty-like.
        // Find longest string in booking.
        longest = 0;
        if((length = strlen(head->start)) > longest) longest = length;
        if((length = strlen(head->end))   > longest) longest = length;
        if((length = strlen(head->name))  > longest) longest = length;
        if((length = strlen(head->phone)) > longest) longest = length;
        if((length = strlen(head->account_number)) > longest) longest = length;
        if((length = strlen(head->email)) > longest) longest = length;
        if((length = strlen(head->info))  > longest) longest = length;
        if((length = strlen(head->time))  > longest) longest = length;
        if((length = strlen(head->flight_number)) > longest) longest = length;
 
        // Print the top line.
        printf(".-%08u-----", head->job_id);
        for(i = 0; i < longest; i++)
            putchar('-');
        putchar('.');
         
        // Print the entries with space padding.
        printf("\n| start     | %s", head->start);
        j = strlen(head->start);
        for(i = j; i < longest; i++)
            putchar(' ');
        puts(" |");
         
        printf("| end       | %s", head->end);
        j = strlen(head->end);
        for(i = j; i < longest; i++)
            putchar(' ');
        puts(" |");
 
        printf("| name      | %s", head->name);
        j = strlen(head->name);
        for(i = j; i < longest; i++)
            putchar(' ');
        puts(" |");
         
        printf("| phone     | %s", head->phone);
        j = strlen(head->phone);
        for(i = j; i < longest; i++)
            putchar(' ');
        puts(" |");
 
        printf("| account # | %s", head->account_number);
        j = strlen(head->account_number);
        for(i = j; i < longest; i++)
            putchar(' ');
        puts(" |");
        
        printf("| email     | %s", head->email);
        j = strlen(head->email);
        for(i = j; i < longest; i++)
            putchar(' ');
        puts(" |");
 
        printf("| info      | %s", head->info);
        j = strlen(head->info);
        for(i = j; i < longest; i++)
            putchar(' ');
        puts(" |");
         
        printf("| time      | %s", head->time);
        j = strlen(head->time);
        for(i = j; i < longest; i++)
            putchar(' ');
        puts(" |");
         
        printf("| flight #  | %s", head->flight_number);
        j = strlen(head->flight_number);
        for(i = j; i < longest; i++)
            putchar(' ');
        puts(" |");
 
        // Print the bottom line.
        printf("\'--------------");
        for(i = 0; i < longest; i++)
            putchar('-');
        puts("\'");
 
        head = head->next;
    }
}
 
 
 
void delete_by_id(struct _booking **head, struct _booking **end, uint32_t id){
/* Deletes the linked list node 
   with the matching job_id.
   
   Needs to include *prev stuff.
*/
 
    struct _booking *scout = *head;
  
    // Check that list exists.
    if(scout == NULL){
        puts("\nList is empty.");
        return;
    }
    
    
    // If root matches, delete it.
    else if((*scout).job_id == id){
        // delete scout content.
        free((*scout).start);
        free((*scout).end);
        free((*scout).name);
        free((*scout).phone);
        free((*scout).account_number);
        free((*scout).email);
        free((*scout).info);
        free((*scout).time);
        free((*scout).flight_number);    
         if((*scout).next)
            (*(*scout).next).prev = NULL;
         *head = (*(*head)).next;
        free(scout);
        puts("\nDeleted by id.");
        return;
    }
     
    // Check all other nodes.
    else{
        scout = (*scout).next;
        
        while(scout != NULL){
            if((*scout).job_id == id){
               
               /* Rewire next and prev.
                  Say I have nodes 1,2, and 3 and I'm deleting 2. 1 needs
                  to point to 3. If 3 is null, then 1 is the new end of
                  the list. if 3 is not null, then 3 needs to point to 1. */
                
                (*(*scout).prev).next = (*scout).next;

                if((*scout).next)
                    (*(*scout).next).prev = (*scout).prev;

                else
                    (*end) = (*scout).prev;
                
                // delete scout content.
                free((*scout).start);
                free((*scout).end);
                free((*scout).name);
                free((*scout).phone);
                free((*scout).account_number);
                free((*scout).email);
                free((*scout).info);
                free((*scout).time);
                free((*scout).flight_number);    
                free(scout);
                
                puts("\nDeleted by id.");
                return;
            }
            else
                scout = (*scout).next;
        }
        puts("\nNot found.");    
    }
}



int send_loop(int sockfd, uint8_t *packet, uint16_t size){
    
    uint16_t totalsent = 0;
    uint16_t bytesleft = size;
    int n;
    
    while(bytesleft){
        n = send(sockfd, packet + totalsent, bytesleft, 0);
        if(n == -1){
            perror("send1");
            break;
        }
        totalsent += n;
        bytesleft -= n;
    }
        
    return n;
}

int recv_loop(int sockfd, uint8_t *packet, uint16_t size){

    uint16_t totalrecv = 0;
    uint16_t bytesleft = size;
    int n;

//    printf("trying to receive %" PRIu16 "bytes on fd %d", bytesleft, sockfd);
    
    while(bytesleft){
        n = recv(sockfd, packet + totalrecv, bytesleft, 0);
        if(n == -1){
            perror("send1");
            break;
        }
        else if(n == 0){
            //socket closed.
            break;
            
            
        }
        totalrecv += n;
        bytesleft -= n;
    }
        
    return n;
}




uint16_t capsule_booking(uint8_t **data, struct _booking *booking){
/* This serialises and encapsulates struct 
   _booking types. First 2 bytes are the size. */
    uint8_t *p8;
    uint16_t *p16;
    uint32_t *p32;
    
    uint16_t packet_size = 2 + 1 + 4 + strlen(booking->start) + strlen(booking->end) + 
                          strlen(booking->name) + strlen(booking->phone) + 
                          strlen(booking->account_number) + strlen(booking->email) + 
                          strlen(booking->info) + strlen(booking->time) + 
                          strlen(booking->flight_number) + 9;
    
    // Make space to hold this data.
    // Include 2 bytes for the size field.
    if((*data = malloc(packet_size)) == NULL){
        puts("\ncapsule_booking(): malloc fail.");
        return -1;
    }

    // Put packet_size into the packet in network byte order.
    p16 = (uint16_t *)(*data);
    *p16 = packet_size - 2;
    
    // Convert this value to network byte order.
    #if BYTE_ORDER == LITTLE_ENDIAN
    *p16 = htons(*p16);
    #endif
    p16++;
    
    // Put struct_id into packet.
    p8 = (uint8_t*)p16;
    *p8 = booking->struct_id; 
    p8++;
    
    // Put the job_id in there.
    p32 = (uint32_t*)p8;
    *p32 = booking->job_id;
    
    // Convert this value to network byte order.
    #if BYTE_ORDER == LITTLE_ENDIAN
    *p32 = htonl(*p32);
    #endif
    p32++;
    
    // Put start into packet.
    p8 = (uint8_t*)p32;
    strcpy(p8, booking->start);
    p8 += 1 + strlen(booking->start);
    
    // Put end into packet.
    strcpy(p8, booking->end);
    p8 += 1 + strlen(booking->end);
    
    // Put name into packet.
    strcpy(p8, booking->name);
    p8 += 1 + strlen(booking->name);

    // Put phone into packet.
    strcpy(p8, booking->phone);
    p8 += 1 + strlen(booking->phone);

    // Put account into packet.
    strcpy(p8, booking->account_number);
    p8 += 1 + strlen(booking->account_number);

    // Put email into packet.
    strcpy(p8, booking->email);
    p8 += 1 + strlen(booking->email);

    // Put info into packet.
    strcpy(p8, booking->info);
    p8 += 1 + strlen(booking->info);

    // Put time into packet.
    strcpy(p8, booking->time);
    p8 += 1 + strlen(booking->time);

    // Put flight into packet.
    strcpy(p8, booking->flight_number);

    // If everything worked, it should look something like this:
    // 030|3|0|Here\0|There\0|Jim\0|555\0|9\0|a@b.com\0|Fish\0|ASAP\0|911\0
    
    return packet_size;
}


struct _booking* find_booking(struct _booking *head, struct _booking *end, uint32_t id){
    /*
    I'll use this when the list is
    doubly linked. Search list for
    matching id, return pointer to
    booking or null if not found.
    */
 
    // Check that list exists.
    if(head == NULL){
         return NULL;
    }
     
    // If root matches, delete it.
    else if((*head).job_id == id){
        return head;
    }
     
    // If end matches, return it.
    else if((*end).job_id == id){
        return end;
    }

    // Otherwise, search list.
    else{
        struct _booking *scout = (*head).next;
        while(scout != NULL){
            if((*scout).job_id == id){
                return scout;
            }
            else
                scout = (*scout).next;
        }
        
        puts("\nNot found.");    
        return NULL;
    }
}


int delete_booking(struct _booking **head, struct _booking **end, struct _booking *target){
    // Given a pointer to a booking in the list, delete it.
    // This function will be used when the list is doubly linked.
    
    puts("\na");
    // Check that list exists.
    if(*head == NULL){
        puts("\nList empty: nothing to delete.");
        return 0;
    }
    
    
    // Check if this is root.
    // If so, move the root.
    else if(target == *head){

        // Make root the node after root.
        *head = (*(*head)).next;

        // If root was the only node, then the node after root is null.
        // If so, we're done. If it's not though, set root's new prev value.
        if(*head)
            (*(*head)).prev = NULL;
    
    }
        
    // Otherwise, do some rewiring.
    else{
        // Point above booking to the below booking.
        (*(*target).prev).next = (*target).next;
        
        // If this is the last booking, move the end pointer.
        if(target == *end){
            *end = (*target).prev;
        }
        
        // If not, point the below booking to the above booking.
        else{
            (*(*target).next).prev = (*target).prev;           
        }
    }
    
    // delete target.
    free((*target).start);
    free((*target).end);
    free((*target).name);
    free((*target).phone);
    free((*target).account_number);
    free((*target).email);
    free((*target).info);
    free((*target).time);
    free((*target).flight_number);    
    free(target);
    return 1;
}