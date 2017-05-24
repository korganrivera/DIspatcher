/*
    nothreadnofork server
   this doesn't use any forking or threading.
   It just processes each connection one at a time.
   I'm guessing that listen blocks enough to handle
   everything sequentially. 
   
   I should count the types of operations I do on the list.
   add, delete, edit, find. Decide if a tree would be 
   more appropriate.
   
   At some point, I need to add some verification that
   the packet that the server receives isn't just some garbage.
figured it out! use a checksum. if checksum is not legit, the packet is garbage.

    Also all packets should be stamped or something to show they came from 
    a legit place.
   
   
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

#define ADD_BOOKING      3
#define SYNC_REQ         4
#define DEL_BOOKING      5
#define EDIT_BOOKING     6
#define DISPATCH_BOOKING 7
#define TAKEBACK_BOOKING 8
#define ALIVE_BEACON     9



struct _booking {
    uint8_t struct_id;          // Use id 3 for now.
    uint32_t job_id;            // Let server set this. Running count.
    char *start;                // 140 bytes is sensible for these.
    char *end;
    char *name;                 // 206. Longest name in the US :)
    char *phone;                // 20
    char *account_number;       // 4 bytes. Why not.
    char *email;                // 254 bytes max. Max length of a possible email.
    char *info;                 // 500 bytes max. Why not.
    char *time;                 // use struct tm later. Use 140 bytes for now.
    char *flight_number;        // XY9999. 8 bytes max.
    struct _booking *next;
};


// Load this from a file later.
uint32_t job_id = 74656;

void hex_string(char *str, uint8_t *data, uint32_t length);
void print_hexstr_to_block(char* str);
char* hex_to_char(uint8_t n);
void decapsule_booking(uint8_t* data, struct _booking *b);
int append(struct _booking **head, struct _booking **end, struct _booking booking);
void show_list(struct _booking* head);
void delete(struct _booking **head, struct _booking **end, uint32_t id);

void sigchld_handler(int s){
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

 /*  TO DO LIST                       | DONE
        ---------------------------------------
        * Sync with clients.          |  
        * Make sure I'm sending back  |
          feedback for all actions.   |  
        * refactor                    |  
        * try select.                 |  
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
    struct _booking booking_buffer;
    struct _booking *curr, *root;
    curr = root = NULL;
    uint32_t new_id, new_idhost;
    
    // Set hints.
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;     // use my IP
 
 
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



    // No forks no threads--Process each connection one at a time.
    
    while(1) { // main accept() loop
        sin_size = sizeof their_addr;
        printf("server: waiting for connections...\n");
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        puts("\n--------------------------------------------------------------------------------");
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        // First thing to do is to receive the front two bytes of the packet.
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
            printf("\n2 bytes received successfully.");
            
            // Convert 2 bytes to a host 
            // order so I can print it.
            #if BYTE_ORDER == LITTLE_ENDIAN
            buffer16host = ntohs(buffer16);
            #endif            
            
            //printf("\nThe 2-byte value is %" PRIu16, buffer16host);
            
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
                puts("\nRest of packet received.");
                
                // Print it to check. Malloc space for the 
                // hex version.
                hexpacket = malloc(buffer16host + 2);
                hex_string(hexpacket, (uint8_t *)packet, buffer16host + 2);
                print_hexstr_to_block(hexpacket); 
        
                
                
                // Find out what type of packet this is: sync request, booking, &c.
                p16 = (uint16_t *)packet;
                p16++;
                p8 = (uint8_t *)p16;
                if(*p8 == SYNC_REQ) {
                    // If packet is a sync request, send all packets in my list.
                    puts("\nPacket is a sync request.");
                    // Send number of packets in the list.
                    struct _booking *scout = root;
                    uint16_t m2, m = 0;
                    while(scout){
                        m++;
                        scout = scout->next;                        
                    }
                    printf("\nThere are %" PRIu16 " packets to send.\n", m);
                    m2 = m;
                    // Send this number first.
                    #if BYTE_ORDER == LITTLE_ENDIAN
                    m = htons(m);
                    #endif
                    
                    // send m.
                    packetsize = 2;         // Length of string including terminator.
                    totalsent = 0;
                    bytesleft = packetsize; 
                    p8 = (uint8_t *)&m;
                    while(totalsent < packetsize){
                        n = send(new_fd, p8 + totalsent, bytesleft, 0);
                        if(n == -1){
                            perror("send1");
                            break;
                        }
                    
                        totalsent += n;
                        bytesleft -= n;
                    }
                    // skip error checking on this for now.
                    // Send all the bookings.
                    scout = root;
                        
                    for(int i = 0; i < m2; i++){                        
                        
                        // encapsulate scout.
                        uint16_t packet_size = 1 + 4 + strlen(scout->start) + strlen(scout->end) + 
                          strlen(scout->name) + strlen(scout->phone) + 
                          strlen(scout->account_number) + strlen(scout->email) + 
                          strlen(scout->info) + strlen(scout->time) + 
                          strlen(scout->flight_number) + 9;
                        
                        // Make space to hold this data.
                        // Include 2 bytes for the size field.
                        free(packet);
                        if((packet = malloc(packet_size + 2)) == NULL){
                            puts("\nmalloc fail 1.");
                            exit(1);
                        }
                        // Put packet_size into the packet in network byte order.
                        p16 = (uint16_t *)packet;
                        *p16 = packet_size;
                                                
                        // Convert this value to network byte order.
                        #if BYTE_ORDER == LITTLE_ENDIAN
                        *p16 = htons(*p16);
                        #endif
                        p16++;
                        
                        // Put struct_id into packet.
                        p8 = (uint8_t*)p16;
                        *p8 = scout->struct_id; 
                        p8++;
                        
                        // Put job_id into packet.
                        uint32_t *p32 = (uint32_t*)p8;
                        *p32 = scout->job_id;
                        
                        // Convert this value to network byte order.
                        #if BYTE_ORDER == LITTLE_ENDIAN
                        *p32 = htonl(*p32);
                        #endif
                        p32++;
                        
                        p8 = (uint8_t*)p32;
                        // Put start into packet.
                        strcpy(p8, scout->start);
                        p8 += 1 + strlen(scout->start);
                        
                        // Put end into packet.
                        strcpy(p8, scout->end);
                        p8 += 1 + strlen(scout->end);
                        
                        // Put name into packet.
                        strcpy(p8, scout->name);
                        p8 += 1 + strlen(scout->name);

                        // Put phone into packet.
                        strcpy(p8, scout->phone);
                        p8 += 1 + strlen(scout->phone);

                        // Put account into packet.
                        strcpy(p8, scout->account_number);
                        p8 += 1 + strlen(scout->account_number);

                        // Put email into packet.
                        strcpy(p8, scout->email);
                        p8 += 1 + strlen(scout->email);

                        // Put info into packet.
                        strcpy(p8, scout->info);
                        p8 += 1 + strlen(scout->info);

                        // Put time into packet.
                        strcpy(p8, scout->time);
                        p8 += 1 + strlen(scout->time);

                        // Put flight into packet.
                        strcpy(p8, scout->flight_number);                                    
                                                
                        // Send packet.
                        uint16_t p_size = packet_size + 2;
                        totalsent = 0;
                        bytesleft = p_size; 
                        int n;

                        while(totalsent < p_size){
                            n = send(new_fd, packet + totalsent, bytesleft, 0);
                            if(n == -1){
                                perror("send1");
                                break;
                            }
                        
                            totalsent += n;
                            bytesleft -= n;
                        }
                        
                        if(n == -1){
                            puts("\nError sending packet.");   
                            break;                            
                        }
                                
                        else{ 
                            printf("\nPacket %d sent.", i);
                        
                        }   
                        scout = scout->next;
                    }
                    
                    // If any syncing happened, report.
                    if(m2 > 0){
                    
                        if(n == -1){
                            puts("\nThere was a sync-send error.");                        
                        }
                        else{
                            puts("\nAll sync done successfully probably.");
                        }
                    }
                }

                else if(*p8 == ADD_BOOKING){ // Packet is a new booking.
                    // Else if packet is a booking, send back id, and deal with packet.
                    printf("\n*p8 = %" PRIu8, *p8);
                    puts("\nPacket is a new booking.");
                    // Send back job_id as receipt.
                    new_id = job_id;
                    new_idhost = new_id;
                    
                    // Increment job_id.
                    if(job_id == 4294967295)
                        job_id = 0;
                    else
                        job_id++;
                    
                    // Should probably check if this id has been used before,
                    // as unlikely as that is.
                    
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
                        
                        // Packet has been received completely. It's a booking.
                        // I need to build it and add it to the server's list here.
 
                        // Wipe booking struct.
                        booking_buffer = (struct _booking){0};
                        // Deserialise the data from where it already is into a booking.

                                               
                                
                        decapsule_booking(packet, &booking_buffer);
                        booking_buffer.job_id = new_idhost;
                        
                        // Append this booking to the list.
                        if(append(&root, &curr, booking_buffer) != 1) 
                            puts("\nappend fucked up.");
                        else puts("\nbooking added to list.");
                        // Print list to check.
                        show_list(root);
                    
                    }         
                }
                
                else if(*p8 == DEL_BOOKING){
                    puts("\nPacket is a delete request");
                    p8++;
                    uint32_t *jobid_to_delete = (uint32_t*)p8;
                    uint32_t jitd = *jobid_to_delete;                    
                                        
                    #if BYTE_ORDER == LITTLE_ENDIAN
                    jitd = ntohl(jitd);
                    #endif
                    
                    printf("\nRequest to delete job %" PRIu32, jitd);
                    
                    // Find and delete booking in the server's list.
                    delete(&root, &curr, jitd);
                    
                    // Print list to check.
                    show_list(root);
                    
                    // Really, delete() needs to return shit, and then I can 
                    // maybe send back feedback to client. But for now,
                    // delete() will delete the thing if found or just not.
                    // delete() will print its own results.
                    
                    // Also need to send a delete request 
                    // to all other connected clients.
                    
                }
                
                else if(*p8 == EDIT_BOOKING){
                    puts("\nPacket is an edit request");
                    // Wipe booking struct.
                    booking_buffer = (struct _booking){0};
                    decapsule_booking(packet, &booking_buffer);
                    // Search the list for a matching job_id.
                    
                    // Search list for this.
                    struct _booking *scout = root;
                    // Check that list exists.
                    if(scout == NULL)
                        puts("\nList is empty.");
                    else{
                        while(scout != NULL){
                            if(scout->job_id == booking_buffer.job_id)
                                break;
                            else
                                scout = scout->next;

                        }               
                        if(scout == NULL)
                            puts("\njob_id not found.");
                        else{                      
                            // Matching job_id found. Replace list
                            // entry with this new booking.
                            
                            
                            
                            
                            
                            
                            
                            // Just delete the old one and put this new one in.
                            delete(&root, &curr, booking_buffer.job_id);
                            append(&root, &curr, booking_buffer);
                            
                            puts("\nList entry updated.");
                            // Print list to check.
                            show_list(root);
                    
                        }
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
        free(packet);
        close(new_fd);  
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
 


 
// This only works when the booking already exists in main().
// I still have to malloc the char pointers though.
// This version only for testing.
void decapsule_booking(uint8_t* data, struct _booking *b){
    uint16_t *p16 = (uint16_t *)data;
    p16++;                          // Skip over first 2 bytes: size.
    uint8_t *p8 = (uint8_t *)p16;   // Point to struct_id. 
    b->struct_id = *p8;             // Copy struct_id.
    p8++;                           // Skip over 1 byte.
    uint32_t *p32 = (uint32_t *)p8; // Point to next 4 bytes: job_id;
    b->job_id = *p32;               // Copy job_id.
     
#if BYTE_ORDER == LITTLE_ENDIAN     // If host machine is little-endian, convert
    b->job_id = ntohl(b->job_id);   // this 4-byte value to little-endian.
#endif
     
    p32++;                          // Skip over 4 bytes.
    p8 = (uint8_t *)p32;            // Point to next byte: Start of 'start'.
     
    uint16_t string_length = strlen(p8);            // Get length of string without null.
    b->start = malloc(string_length + 1);  // Make space for it.
    strcpy(b->start, p8);                  // Copy string into b inc. null.
    p8 += string_length + 1;                        // Move to next string: 'end'.
 
    string_length = strlen(p8);                     // Get length of string without null.
    b->end = malloc(string_length + 1);    // Make space for it.
    strcpy(b->end, p8);                    // Copy string into b inc. null.
    p8 += string_length + 1;                        // Move to next string: 'name'.
     
    string_length = strlen(p8);                     // Get length of string without null.
    b->name = malloc(string_length + 1);            // Make space for it.
    strcpy(b->name, p8);                            // Copy string into b inc. null.
    p8 += string_length + 1;                        // Move to next string: 'phone'.
     
    string_length = strlen(p8);                     // Get length of string without null.
    b->phone = malloc(string_length + 1);           // Make space for it.
    strcpy(b->phone, p8);                           // Copy string into b inc. null.
    p8 += string_length + 1;                        // Move to next string: 'account_number'.
 
    string_length = strlen(p8);                     // Get length of string without null.
    b->account_number = malloc(string_length + 1);  // Make space for it.
    strcpy(b->account_number, p8);            // Copy string into b inc. null.
    p8 += string_length + 1;                        // Move to next string: 'email'.
     
    string_length = strlen(p8);                     // Get length of string without null.
    b->email = malloc(string_length + 1);           // Make space for it.
    strcpy(b->email, p8);                     // Copy string into b inc. null.
    p8 += string_length + 1;                        // Move to next string: 'info'.
     
    string_length = strlen(p8);                     // Get length of string without null.
    b->info = malloc(string_length + 1);            // Make space for it.
    strcpy(b->info, p8);                      // Copy string into b inc. null.
    p8 += string_length + 1;                        // Move to next string: 'time'.
 
    string_length = strlen(p8);                     // Get length of string without null.
    b->time = malloc(string_length + 1);            // Make space for it.
    strcpy(b->time, p8);                      // Copy string into b inc. null.
    p8 += string_length + 1;                        // Move to next string: 'flight_num'.
 
    string_length = strlen(p8);                     // Get length of string without null.
    b->flight_number = malloc(string_length + 1);      // Make space for it.
    strcpy(b->flight_number, p8);                // Copy string into b inc. null.   
} 



 
int append(struct _booking **head, struct _booking **end, struct _booking booking){
/* Appends a booking to the end of the linked
   list. If no list exists, it initiates one.
*/

    struct _booking *current;

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
        *end = (*end)->next;
    } 
    
    current = *end;
    *current = booking;
    (*current).next = NULL;
    
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




void delete(struct _booking **head, struct _booking **end, uint32_t id){
/* Deletes the linked list node 
   with the matching job_id.
*/

    struct _booking *scout = *head;

    // Check that list exists.
    if(scout == NULL){
        puts("\nList is empty.");
        return;
    }
    
    // If root matches, delete it.
    else if((*scout).job_id == id){
        *head = (*(*head)).next;
        free(scout);
        puts("\nDeleted.");
        return;
    }
    
    // Check all other nodes.
    else{
        while((*scout).next != NULL){
            if((*((*scout).next)).job_id == id){
                struct _booking *temp = (*scout).next;
                (*scout).next = (*((*scout).next)).next;
                free(temp);
                if((*scout).next == NULL)
                    *end = scout;
                puts("\nDeleted.");
                return;
            }
            else
                scout = (*scout).next;
        }
        puts("\nNot found.");    
    }
}