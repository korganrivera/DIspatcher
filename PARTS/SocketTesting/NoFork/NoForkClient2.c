/*

    NoForkClient2.c 
   
    IF I START GETTING WEIRD POINTER ERRORS, then revert back to NoForkClient1
    and burn down the house!

   I'm going to turn this into my proper client.
   Something is fucky with the other version.
   This is going to a monolithic son-of-a-bitch.
 

This is a list of super-basic tasks that I have to be able to do:
    1. Sync the dispatcher with the server when the dispatcher connects.
       This means the job list, and the cab status.
    2. Make a driver program. Be able to send jobs to a driver, have the driver confirm or deny.
    3. Sync the driver with the server when the driver disconnects.
       If a driver logs out then logs in, his cab's job list has to be resent to him.

       
       
       
       
       Something is wrong with delete_booking
       
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
    struct _booking *prev;
};

/*
    Later on, I'll have to expand the booking 
    structure to include the following:
    
        cab
        driver
        started
        picked up
        dropped off
        assigned
        created by
        source
        
    I think job events will be kept 
    on the server though. Not sure.
*/




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
void decapsule_booking(uint8_t* data, struct _booking *b);
uint16_t capsule_booking(uint8_t **data, struct _booking *booking);
int send_loop(int sockfd, uint8_t *packet, uint16_t size);
int recv_loop(int sockfd, uint8_t *packet, uint16_t size);
struct _booking* find_booking(struct _booking *head, struct _booking *end, uint32_t id);
int delete_booking(struct _booking **head, struct _booking **end, struct _booking *target);

void print_blob(uint8_t *blob, uint16_t size){
    uint16_t i;
    
    printf("\n0x");
    for(i = 0; i < size; i++)
        printf("%" PRIx8, blob[i]);
}


int main(void){

    /*  TO DO LIST                         | DONE
        -----------------------------------------
        * Make structure.                  | Y
        * Turn it into a packet.           | Y
        * Send it.                         | Y
        * Receive job_id.                  | Y
        * put job_id in structure.         | Y
        * Add structure to my linked list. | Y
        * Loop the options.                | Y
        * Add in other menu options:       |  
            - Delete                       | Y
            - Sync                         | Can sync on client startup, but otherwise can't.
            - Edit                         | Y
            - Dispatch                     |  
            - Takeback                     |      
        * Send GPS beacons                 |     
        * Fix string resize during edit.   | Y     
        * use the capsule_booking          | Y
          function.                        |  
    
    */

    struct _booking booking_buffer, *curr, *root;
    int i;
    uint8_t  *p8, *packet;
    uint16_t *p16, packet_size;
    uint32_t *p32;    
        
    curr = root = NULL;
    
    /* First thing I have to do is connect to 
       the server, and sync the jobs list. */

    // Get a socket descriptor.------------------------------------------------
    puts("\nConnecting to server...");
    int sockfd = get_sockfd(HOST, PORT);
    if(sockfd < 0){
        puts("\nSomething is fucky. Can't\nsync. Is the server down?");
        exit(1);
    }

    else{
        
        puts("\n88        88            88 88\n"
               "88        88            88 88\n"
               "88        88            88 88\n"
               "88aaaaaaaa88  ,adPPYba, 88 88  ,adPPYba,\n"
               "88\"\"\"\"\"\"\"\"88 a8P_____88 88 88 a8\"     \"8a\n"
               "88        88 8PP\"\"\"\"\"\"\" 88 88 8b       d8\n"
               "88        88 \"8b,   ,aa 88 88 \"8a,   ,a8\" 888\n"
               "88        88  `\"Ybbd8\"' 88 88  `\"YbbdP\"'  888");
        
        // Send server a sync request.-----------------------------------------
        // | packet size | packet type | 
        // |---2 bytes---|---1 byte----|
        // |      1      |  SYNC_REQ   |
        packet_size = 3;

        // Make space to hold this data.---------------------------------------
        if((packet = malloc(packet_size)) == NULL){
            puts("\nmalloc fail: sync request.");
            exit(1);
        }
    
        // Put packet_size into the packet in network byte order.--------------
        p16 = (uint16_t *)packet;
        *p16 = 1;
        
        // Convert this value to network byte order.---------------------------
        #if BYTE_ORDER == LITTLE_ENDIAN
        *p16 = htons(*p16);
        #endif
        p16++;
        
        // Put struct_id into packet.------------------------------------------
        p8 = (uint8_t*)p16;
        *p8 = SYNC_REQ;
        p8++;
        
        // Print it to check.
        puts("\nsync request packet check:");
        char hexstr[packet_size];
        hex_string(hexstr, packet, packet_size);
        print_hexstr_to_block(hexstr);

        // Send packet: sync request.------------------------------------------
        if((send_loop(sockfd, packet, packet_size)) == -1){
            puts("\nError sending packet.");                 
        }

        else{        
            puts("\nsync request sent successfully.");
            
            /* Receive a stream of packets. The first thing I receive----------
            is the number of packets about to be sent. Then I loop
            the usual receive code until I have them all. */

            uint16_t buffer16, buffer16host;
            uint16_t *p16, packetsize = 2;
            uint8_t *p8 = (uint8_t *)&buffer16;
            int n;
            if((n = recv_loop(sockfd, p8, packetsize)) == -1){
                puts("\nError receiving packet.");
            }
            
            else{
                printf("\n2 bytes received successfully.");
                
                // Convert 2 bytes to a host 
                // order so I can print it.
                buffer16host = buffer16;
                #if BYTE_ORDER == LITTLE_ENDIAN
                buffer16host = ntohs(buffer16host);
                #endif            
                printf("\nServer is sending %" PRIu16 " packets.", buffer16host);

                // Setup a loop to receive this many packets.------------------
                uint16_t count = buffer16host;
                for(i = 0; i < count; i++){
                    
                    // Get two bytes (size), read them into buffer16.----------
                    packetsize = 2;
                    p8 = (uint8_t *)&buffer16;
                    if((n = recv_loop(sockfd, p8, packetsize)) == -1){
                        puts("\nError receiving packet.");
                    }
                    
                    else{
                        printf("\nPacket %d:", i + 1);
                        // Convert 2 bytes to host-----------------------------
                        // order so I can print it.
                        buffer16host = buffer16;
                        #if BYTE_ORDER == LITTLE_ENDIAN
                        buffer16host = ntohs(buffer16host);
                        #endif            
                        printf("\nsize: %" PRIu16, buffer16host);
                                    
                        // Now I need to malloc space for the rest-------------
                        // of the packet, plus the first two bytes.
                        free(packet);
                        packet = malloc(buffer16host + 2);                                   
                        
                        // Put first two bytes into the packet.----------------
                        p16 = (uint16_t *)packet;
                        *p16 = buffer16;
                        p16++;
                        p8 = (uint8_t *)p16;
                        
                        // Read rest of packet.--------------------------------
                        packetsize = buffer16host;
                        if((n = recv_loop(sockfd, p8, packetsize)) == -1){
                            puts("\nError receiving packet.");
                        }
                        
                        else{

                            // Got the packet. Wipe booking struct.
                            booking_buffer = (struct _booking){0};
                            
                            // Deserialise the data from where
                            // it already is into a booking.
                            decapsule_booking(packet, &booking_buffer);
                            
                            // Append this booking to the list.
                            if(append(&root, &curr, booking_buffer) != 1) 
                                puts("\nappend fucked up.");
                            else puts("\nbooking added to list.");
                        }
                    }//else
                        
                    if(n == -1){
                        puts("\nOne or more of packets failed to sync.");
                        break;                           
                    }
                }//for
                
                //-------------------------------------------------------------
                if(count == 0) puts("\nServer's list empty. No sync needed.");
                else if(i == count) puts("\nSync successful.");
                else puts("\nSync failed. :(");
            }                        
        }
        free(packet);
        close(sockfd);
    }//else        
    
    // After sync, run the main loop.------------------------------------------
    do{
      
        show_menu();
        enum {ADD = 1, DELETE, DISPATCH, EDIT, SHOW, QUIT} ch;
        ch = get_choice();

        //---------------------------------------------------------------------
        if(ch == ADD){
            // Get booking details from the user.
            // First, set the struct id to 3. 
            booking_buffer.struct_id = ADD_BOOKING;
            
            // Set the job_id to zero for now.
            booking_buffer.job_id = 0;
            
            // Get booking details from user.----------------------------------
            // Read the start location into the character buffer.
            printf("\nlocation start: ");
            get_string(&booking_buffer.start, 1024);
            
            // Read the end location into the character buffer.
            printf("\nlocation end: ");
            get_string(&booking_buffer.end, 1024);

            // Read the name into the character buffer.
            printf("\nname: ");
            get_string(&booking_buffer.name, 1024);

            // Read the phone number into the character buffer.
            printf("\nphone #: ");
            get_string(&booking_buffer.phone, 1024);
            
            // Read the account number into the character buffer.
            printf("\naccount number: ");
            get_string(&booking_buffer.account_number, 1024);
            
            // Read the email into the character buffer.
            printf("\nemail: ");
            get_string(&booking_buffer.email, 1024);
            
            // Read the info into the character buffer.
            printf("\ninfo: ");
            get_string(&booking_buffer.info, 1024);
            
            // Read the time into the character buffer.
            printf("\ntime: ");
            get_string(&booking_buffer.time, 1024);

            // Read the flight into the character buffer.
            printf("\nflight number: ");
            get_string(&booking_buffer.flight_number, 1024);
            
            // Set next to NULL.
            booking_buffer.next = NULL;
            booking_buffer.prev = NULL;

            /* Make a packet version of booking_buffer. First,
               calculate the size of the packet in bytes. This
               is the size of all the members of booking_buffer
               plus 2 bytes that tell the packet size.      */
               
            // | packet size | packet type |   job_id    |    strings...    |    
            // |---2 bytes---|---1 byte----|---4 bytes---|-----n bytes------|    
            // |    n + 2    |  SYNC_REQ   |  <job_id>   |  str1\0str2\0... |
            
            packet_size = capsule_booking(&packet, &booking_buffer);
            printf("packet size: %" PRIu16 "\n", packet_size);
               
            // Print it to check.
            puts("\nPacket block");
            char hexstr[packet_size];
            hex_string(hexstr, packet, packet_size);
            print_hexstr_to_block(hexstr);
            puts("\nEnd packet block");
            
            // Now I send it.--------------------------------------------------
            // Get a socket descriptor.
            int sockfd = get_sockfd(HOST, PORT);
            if(sockfd < 0){
                puts("\nSomething is fucky. Can't\nsend. Is server down?");
            }

            else{
                // Send packet: add booking.
                if((send_loop(sockfd, packet, packet_size)) == -1){
                    puts("\nError sending packet.");                 
                }
        
                else{                 
                    puts("\nPacket sent successfully.");
                    
                    // Get a receipt: job_id.----------------------------------
                    uint32_t receipt;
                    uint8_t *p8 = (uint8_t *)&receipt;
                    int n;
                    
                    if((n = recv_loop(sockfd, p8, 4)) == -1){
                        puts("\nError receiving packet.");
                    }
                            
                    else{                        
                        #if BYTE_ORDER == LITTLE_ENDIAN
                        receipt = ntohl(receipt);
                        #endif
                        printf("\njob_id received = %" PRIu32, receipt);
                        
                        // Put job_id into booking_buffer.---------------------
                        booking_buffer.job_id = receipt;
                        
                        // Append booking_buffer to the linked list.-----------
                        if(append(&root, &curr, booking_buffer) != 1) 
                            puts("\nappend fucked up.");
                        else puts("\nbooking added to list.");
                     }
                }
                free(packet);
                close(sockfd);
            }
        }
        
        //---------------------------------------------------------------------
        else if(ch == DELETE){
            uint32_t id;
            
            // I need to see if this id exists before bothering to send a packet.
            
            
            if(root == NULL)
                puts("\nEmpty list: nothing to delete.");
            
            
            
            else{
                printf("Enter job_id to delete: ");
                scanf("%" SCNu32, &id);
                getchar();
                                
                // Make sure this id exists.
                struct _booking *scout = find_booking(root, curr, id);
                
                if(scout == NULL){
                    puts("\nid not found locally.");
                }

                else{
                    // Before deleting it locally, 
                    // Send a delete to the server.

                    // Send server a delete request.
                    // | packet size | packet type |    job_id    |
                    // |---2 bytes---|---1 byte----|--- 4 bytes---|
                    // |      5      | DEL_BOOKING |   <job_id>   |
                    packet_size = 7;

                    // Make space to hold this data.
                    // Include 2 bytes for the size field.
                    if((packet = malloc(packet_size)) == NULL){
                        puts("\nmalloc fail: delete request.");
                        exit(1);
                    }
                
                    // Put packet_size into the packet in network byte order.
                    p16 = (uint16_t *)packet;
                    *p16 = 5;
                    
                    // Convert this value to network byte order.
                    #if BYTE_ORDER == LITTLE_ENDIAN
                    *p16 = htons(*p16);
                    #endif
                    p16++;
                    
                    // Put struct_id into packet.
                    p8 = (uint8_t*)p16;
                    *p8 = DEL_BOOKING;
                    p8++;
                    
                    p32 = (uint32_t*)p8;
                    *p32 = id;
                    
                    #if BYTE_ORDER == LITTLE_ENDIAN
                    *p32 = htonl(*p32);
                    #endif
                    
                    // Print it to check.------------------------------------------
                    puts("\ndelete request packet check:");
                    char hexstr[packet_size];
                    hex_string(hexstr, packet, packet_size);
                    print_hexstr_to_block(hexstr);

                    // First I need to connect.
                    int sockfd = get_sockfd(HOST, PORT);
                    if(sockfd < 0){
                        puts("\nSomething is fucky. Can't connect to"
                             "\nsend delete request. Is server down?");
                        exit(1);
                    }

                    else{                
                        
                        // Send packet: delete booking.
                        if((send_loop(sockfd, packet, packet_size)) == -1){
                            puts("\nError sending packet.");                 
                        }
                
                        else{                      
                            puts("\ndelete request sent successfully.");
                     
                            // Since server deleted successfully, 
                            // I can go ahead and delete locally.
                            puts("\nboop");
                            delete_booking(&root, &curr, scout);
                            puts("\nboop");
                        }    
                        free(packet);
                    }  
                }                    
            }
        }
        //---------------------------------------------------------------------
        else if(ch == DISPATCH){
            puts("\nI can't dispatch yet.");
            
            /*
            Can't write this until I have a driver client.
            */
            
        }
        
        //---------------------------------------------------------------------
        else if(ch == EDIT){
                        
            /*  Ask user for id of booking he wants to edit. Search list for
                this id. If list empty, say list empty. If id not found, say
                not found. If found, copy booking into buffer. go through
                booking with user, if user presses enter, keep field as it
                is. if user enters a string, replace string in buffer with
                what user enters. If string requires resizing, do it. After
                this, send it to server with EDIT_BOOKING type. If server
                successful, update local copy. (delete and append new one.)
            */              
            
            uint32_t id;
            if(root == NULL)
                puts("\nEmpty list: nothing to edit.");
       
            else{
                // Ask user which booking to edit.       
                printf("Enter job_id to edit: ");
                scanf("%" SCNu32, &id);
                getchar();
            
                // Search list for this.
                struct _booking *scout = root;
               
               // Check that list exists.
                if(scout == NULL)
                    puts("\nList is empty.");
               
                else{
              
                    // Search for a match.
                    while(scout != NULL){
                        if(scout->job_id == id)
                            break;
                        else
                            scout = scout->next;

                    }   
                    
                    if(scout == NULL)
                        puts("\njob_id not found.");
                  
                    else{ 

                        char *str_buff;

                        // Copy matching booking into booking_buffer.
                        booking_buffer = *scout;
                        booking_buffer.struct_id = EDIT_BOOKING;
                        
                        // Go through values and update as necessary,
                        puts("\nPress enter to keep, or enter new value.");
                        
                        printf("\nlocation start [%s]: ", booking_buffer.start);
                        get_string(&str_buff, 1024);
                        if(*str_buff != '\0'){
                            // If user entered a new value, measure it's length first.
                            int bufflen = strlen(str_buff);
                            
                            // Check if this is longer than the current string.
                            // If so, resize the current string to accomodate.
                            if(bufflen > strlen(booking_buffer.start)){
                                realloc(booking_buffer.start, bufflen + 1);                               
                            }

                            // Copy the new string over the old string.
                            strcpy(booking_buffer.start, str_buff);
                        }

                        // If str_buff was null, leave the value as it is.                        
                        // get_string mallocs, so I need to free str_buff.
                        free(str_buff);
                        
                        // Do the same as above 8 more times.
                        printf("\nlocation end [%s]: ", booking_buffer.end);
                        get_string(&str_buff, 1024);
                        
                        if(*str_buff != '\0'){
                            int bufflen = strlen(str_buff);
                            if(bufflen > strlen(booking_buffer.end)){
                                realloc(booking_buffer.end, bufflen + 1);                               
                            }
                            strcpy(booking_buffer.end, str_buff);
                        }
                        free(str_buff);
                        
                        printf("\nname [%s]: ", booking_buffer.name);
                        get_string(&str_buff, 1024);
                        if(*str_buff != '\0'){
                            int bufflen = strlen(str_buff);
                            if(bufflen > strlen(booking_buffer.name)){
                                realloc(booking_buffer.name, bufflen + 1);                               
                            }
                            strcpy(booking_buffer.name, str_buff);
                        }
                        free(str_buff);
                        
                        printf("\nphone #[%s]: ", booking_buffer.phone);
                        get_string(&str_buff, 1024);
                        if(*str_buff != '\0'){
                            int bufflen = strlen(str_buff);
                            if(bufflen > strlen(booking_buffer.phone)){
                                realloc(booking_buffer.phone, bufflen + 1);                               
                            }
                            strcpy(booking_buffer.phone, str_buff);
                        }
                        free(str_buff);
                        
                        printf("\naccount # [%s]: ", booking_buffer.account_number);
                        get_string(&str_buff, 1024);
                        if(*str_buff != '\0'){
                            int bufflen = strlen(str_buff);
                            if(bufflen > strlen(booking_buffer.account_number)){
                                realloc(booking_buffer.account_number, bufflen + 1);                               
                            }
                            strcpy(booking_buffer.account_number, str_buff);
                        }
                        free(str_buff);
                        
                        
                        printf("\nemail [%s]: ", booking_buffer.email);
                        get_string(&str_buff, 1024);
                        if(*str_buff != '\0'){
                            int bufflen = strlen(str_buff);
                            if(bufflen > strlen(booking_buffer.email)){
                                realloc(booking_buffer.email, bufflen + 1);                               
                            }
                            strcpy(booking_buffer.email, str_buff);
                        }
                        free(str_buff);
                        
                        
                        printf("\ninfo [%s]: ", booking_buffer.info);
                        get_string(&str_buff, 1024);
                        if(*str_buff != '\0'){
                            int bufflen = strlen(str_buff);
                            if(bufflen > strlen(booking_buffer.info)){
                                realloc(booking_buffer.info, bufflen + 1);                               
                            }
                            strcpy(booking_buffer.info, str_buff);
                        }
                        free(str_buff);
                        
                        
                        printf("\ntime [%s]: ", booking_buffer.time);
                        get_string(&str_buff, 1024);
                        if(*str_buff != '\0'){
                            int bufflen = strlen(str_buff);
                            if(bufflen > strlen(booking_buffer.time)){
                                realloc(booking_buffer.time, bufflen + 1);                               
                            }
                            strcpy(booking_buffer.time, str_buff);
                        }
                        free(str_buff);
                        
                        printf("\nflight # [%s]: ", booking_buffer.flight_number);
                        get_string(&str_buff, 1024);
                        if(*str_buff != '\0'){
                            int bufflen = strlen(str_buff);
                            if(bufflen > strlen(booking_buffer.flight_number)){
                                realloc(booking_buffer.flight_number, bufflen + 1);                               
                            }
                            strcpy(booking_buffer.flight_number, str_buff);
                        }
                        free(str_buff);
                        
                        booking_buffer.next = NULL;
                        booking_buffer.prev = NULL;
                    
                        // Make a packet version of booking_buffer.
                        packet_size = capsule_booking(&packet, &booking_buffer);
                        printf("packet size: %" PRIu16 "\n", packet_size);                    
                  
                        // Print it to check.----------------------------------------------
                        char hexstr[packet_size];
                        hex_string(hexstr, packet, packet_size);
                        print_hexstr_to_block(hexstr);
                        
                        // Now I send it.--------------------------------------------------
                        // Get a socket descriptor.
                        int sockfd = get_sockfd(HOST, PORT);
                        if(sockfd < 0){
                            puts("\nSomething is fucky. Can't\nsend. Is server down?");
                        }

                        else{
                           
                            if((send_loop(sockfd, packet, packet_size)) == -1){
                                puts("\nError sending packet.");                 
                            }
                                
                            else{ 
                                puts("\nPacket sent successfully.");
                                
                                // Now that booking has changed, I can delete 
                                // the old one and append the new one.
                                delete(&root, &curr, booking_buffer.job_id);
                                if(append(&root, &curr, booking_buffer) != 1) 
                                    puts("\nappend fucked up.");
                                else puts("\nbooking added to list.");
                            }
                            free(packet);
                            close(sockfd);
                        } // else
                    } // else
                }//else
            } //else list not empty.                
        }
        
        else if(ch == SHOW){
            show_list(root);
        }
        
        else if(ch == QUIT){
            puts("\nGoodbye.");
            return 0;
        }
        
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
        "00", "01", "02", "03", "04", "05", "06", "07", 
        "08", "09", "0A", "0B", "0C", "0D", "0E", "0F", 
        "10", "11", "12", "13", "14", "15", "16", "17", 
        "18", "19", "1A", "1B", "1C", "1D", "1E", "1F", 
        "20", "21", "22", "23", "24", "25", "26", "27", 
        "28", "29", "2A", "2B", "2C", "2D", "2E", "2F", 
        "30", "31", "32", "33", "34", "35", "36", "37", 
        "38", "39", "3A", "3B", "3C", "3D", "3E", "3F", 
        "40", "41", "42", "43", "44", "45", "46", "47", 
        "48", "49", "4A", "4B", "4C", "4D", "4E", "4F", 
        "50", "51", "52", "53", "54", "55", "56", "57", 
        "58", "59", "5A", "5B", "5C", "5D", "5E", "5F", 
        "60", "61", "62", "63", "64", "65", "66", "67", 
        "68", "69", "6A", "6B", "6C", "6D", "6E", "6F", 
        "70", "71", "72", "73", "74", "75", "76", "77", 
        "78", "79", "7A", "7B", "7C", "7D", "7E", "7F", 
        "80", "81", "82", "83", "84", "85", "86", "87", 
        "88", "89", "8A", "8B", "8C", "8D", "8E", "8F", 
        "90", "91", "92", "93", "94", "95", "96", "97", 
        "98", "99", "9A", "9B", "9C", "9D", "9E", "9F", 
        "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", 
        "A8", "A9", "AA", "AB", "AC", "AD", "AE", "AF", 
        "B0", "B1", "B2", "B3", "B4", "B5", "B6", "B7", 
        "B8", "B9", "BA", "BB", "BC", "BD", "BE", "BF", 
        "C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7", 
        "C8", "C9", "CA", "CB", "CC", "CD", "CE", "CF", 
        "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", 
        "D8", "D9", "DA", "DB", "DC", "DD", "DE", "DF", 
        "E0", "E1", "E2", "E3", "E4", "E5", "E6", "E7", 
        "E8", "E9", "EA", "EB", "EC", "ED", "EE", "EF", 
        "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", 
        "F8", "F9", "FA", "FB", "FC", "FD", "FE", "FF"
    };
     
    return hex_to_char[n];
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
        if((length = strlen(head->end)) > longest) longest = length;
        if((length = strlen(head->name)) > longest) longest = length;
        if((length = strlen(head->phone)) > longest) longest = length;
        if((length = strlen(head->account_number)) > longest) longest = length;
        if((length = strlen(head->email)) > longest) longest = length;
        if((length = strlen(head->info)) > longest) longest = length;
        if((length = strlen(head->time)) > longest) longest = length;
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
 
 
 

int get_choice(void){
/* Only reads first character of user's input.
   So if you enter 123, it thinks you entered 1.
   Not a big deal, but might want to fix it. */
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




void get_string(char **str, unsigned size){
    char buffer[size];
    int i, str_len;
    
    fgets(buffer, size, stdin);
    
    // Measure the length of the string in buffer
    // including space for the terminator.
    for(i = 0; buffer[i] && buffer[i] != '\n'; i++);
    str_len = i + 1;

    // Malloc custom-sized space for this string.
    *str = malloc(str_len);
    
    // Copy buffer contents into string.
    for(i = 0; buffer[i] && buffer[i] != '\n'; i++)
        (*str)[i] = buffer[i];
         
    // Null terminate this string.
    (*str)[i] = '\0';    
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


// This serialises and encapsulates struct 
// _booking types. First 2 bytes are the size.
uint16_t capsule_booking(uint8_t **data, struct _booking *booking){
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




int send_loop(int sockfd, uint8_t *packet, uint16_t size){
/* Sends packet of the given size
   on the socket sockfd. Loops to
   make sure all bytes get sent.
*/    
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
/*  Recv packet of the given size
    from the socket sockfd. Loops to
    make sure all bytes arrive.
*/    
    uint16_t totalrecv = 0;
    uint16_t bytesleft = size;
    int n;
    
    while(bytesleft){
        n = recv(sockfd, packet + totalrecv, bytesleft, 0);
        if(n == -1){
            perror("send1");
            break;
        }
        totalrecv += n;
        bytesleft -= n;
    }
        
    return n;
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