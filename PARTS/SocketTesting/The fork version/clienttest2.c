/*
   I'm going to turn this into my proper client.
   Something is fucky with the other version.
   This is going to a monolithic son-of-a-bitch.
  
Found this example online. Looks good:
void ListDelete(nodeT **listP, elementT value)
{
  nodeT *currP, *prevP;

  // For 1st node, indicate there is no previous. 
  prevP = NULL;

  
   // Visit each node, maintaining a pointer to
   // the previous node we just visited.
   
  for (currP = *listP;
	currP != NULL;
	prevP = currP, currP = currP->next) {

    if (currP->element == value) {  // Found it. 
      if (prevP == NULL) {
        // Fix beginning pointer. 
        *listP = currP->next;
      } else {
        
         //Fix previous node's next to
         // skip over the removed node.
         
        prevP->next = currP->next;
      }

      // Deallocate the node. 
      free(currP);

      // Done searching. 
      return;
    }
  }
}


This is a list of super-basic tasks that I have to be able to do:
    1. Sync the dispatcher with the server when the dispatcher connects.
        This means the job list, and the cab status.
    2. Make a driver program. Be able to send jobs to a driver, have the driver confirm or deny.
    3. Sync the driver the server when the driver disconnects.
        If a driver logs out then logs in, his cab's job list has to be resent to him.
        
  
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

    

    /*  TO DO LIST                       | DONE
        ---------------------------------------
        Make structure.                  | Y
        Turn it into a packet.           | Y
        Send it.                         | Y
        Receive job_id.                  | Y
        put job_id in structure.         | Y
        Add structure to my linked list. | Y
        Loop the options.                | Y
        Add in other menu options:
            Delete                         Y
            Sync
            Edit
            Dispatch
        
    
    
    */

    struct _booking booking_buffer;
    struct _booking *curr, *root;
    int i, size = 1024;
    char buffer[size], *str;
    uint16_t str_len, packet_size, *p16;
    uint8_t *packet, *p8;
    uint32_t *p32;    
    
    curr = root = NULL;
    
        /* First thing I have to do is connect to 
           the server, and sync the jobs list. */
        // Get a socket descriptor.
        int sockfd = get_sockfd(HOST, PORT);
        if(sockfd < 0){
            puts("\nSomething is fucky. Can't\nsync. Is server down?");
            exit(1);
        }

        else{
            // Send server a sync request.
            // | packet size | packet type | 
            // |---2 bytes---|---1 byte----|
            
            packet_size = 1;

            // Make space to hold this data.
            // Include 2 bytes for the size field.
            if((packet = malloc(packet_size + 2)) == NULL){
                puts("\nmalloc fail: sync request.");
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
            *p8 = 4;    // Use enum later. For now, 4 mean this is a sync request. 
            p8++;
            
            // Print it to check.----------------------------------------------
            puts("\nsync request packet check:");
            char hexstr[packet_size + 2];
            hex_string(hexstr, packet, packet_size + 2);
            print_hexstr_to_block(hexstr);

//----------------------------------------------------------------------------
            
            // Send packet.
            uint16_t p_size = packet_size + 2;
            uint16_t totalsent = 0;
            uint16_t bytesleft = p_size; 
            int n;

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
                free(packet);
            }
                    
            else{        
                puts("\nsync request sent successfully.");
                free(packet);
                // This is where I receive a stream of packets.
                // I suppose the first thing I need to receive is the number
                // of packets about to be sent. Then I just loop the usual
                // receive code until I have that many packets.

            }
            close(sockfd);
        }//else        
//----------------------------------------------------------------------------

    do{
      
        show_menu();
        char ch = get_choice();
        
        if(ch == 1){
            // Get booking details from the user.
            // First, set the struct id to 3. 
            booking_buffer.struct_id = 3;
            
            // Read the start location into the character buffer.--------------
            printf("\nlocation start: ");
            get_string(&booking_buffer.start, 1024);
            
            // Read the end location into the character buffer.----------------
            printf("\nlocation end: ");
            get_string(&booking_buffer.end, 1024);

            // Read the name into the character buffer.------------------------
            printf("\nname: ");
            get_string(&booking_buffer.name, 1024);

            // Read the phone number into the character buffer.----------------
            printf("\nphone #: ");
            get_string(&booking_buffer.phone, 1024);
            
            // Read the account number into the character buffer.--------------
            printf("\naccount number: ");
            get_string(&booking_buffer.account_number, 1024);
            
            // Read the email into the character buffer.-----------------------
            printf("\nemail: ");
            get_string(&booking_buffer.email, 1024);
            
            // Read the info into the character buffer.------------------------
            printf("\ninfo: ");
            get_string(&booking_buffer.info, 1024);
            
            // Read the time into the character buffer.------------------------
            printf("\ntime: ");
            get_string(&booking_buffer.time, 1024);

            // Read the flight into the character buffer.----------------------
            printf("\nflight number: ");
            get_string(&booking_buffer.flight_number, 1024);
            
            // Set next to NULL.-----------------------------------------------
            booking_buffer.next = NULL;

            // Make a packet version of booking_buffer.
            // First, calculate the size of the packet in bytes.
            // This is the size of all the members of booking_buffer.
            packet_size = 1 + 4 + strlen(booking_buffer.start) + strlen(booking_buffer.end) + 
                          strlen(booking_buffer.name) + strlen(booking_buffer.phone) + 
                          strlen(booking_buffer.account_number) + strlen(booking_buffer.email) + 
                          strlen(booking_buffer.info) + strlen(booking_buffer.time) + 
                          strlen(booking_buffer.flight_number) + 9;

            printf("\npacket size will be %" PRIu16 "\n", packet_size);
            printf("adding 2 bytes for the size makes it %d\n", packet_size + 2);
            
            // Make space to hold this data.
            // Include 2 bytes for the size field.
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
            *p8 = booking_buffer.struct_id; 
            p8++;
            
            // Skip the job_id for now.
            p8 += 4;
            
            // Put start into packet.
            strcpy(p8, booking_buffer.start);
            p8 += 1 + strlen(booking_buffer.start);
            
            // Put end into packet.
            strcpy(p8, booking_buffer.end);
            p8 += 1 + strlen(booking_buffer.end);
            
            // Put name into packet.
            strcpy(p8, booking_buffer.name);
            p8 += 1 + strlen(booking_buffer.name);

            // Put phone into packet.
            strcpy(p8, booking_buffer.phone);
            p8 += 1 + strlen(booking_buffer.phone);

            // Put account into packet.
            strcpy(p8, booking_buffer.account_number);
            p8 += 1 + strlen(booking_buffer.account_number);

            // Put email into packet.
            strcpy(p8, booking_buffer.email);
            p8 += 1 + strlen(booking_buffer.email);

            // Put info into packet.
            strcpy(p8, booking_buffer.info);
            p8 += 1 + strlen(booking_buffer.info);

            // Put time into packet.
            strcpy(p8, booking_buffer.time);
            p8 += 1 + strlen(booking_buffer.time);

            // Put flight into packet.
            strcpy(p8, booking_buffer.flight_number);
            
            // Print it to check.----------------------------------------------
            char hexstr[packet_size + 2];
            hex_string(hexstr, packet, packet_size + 2);
            print_hexstr_to_block(hexstr);
            
            // Now I send it.--------------------------------------------------
            // Get a socket descriptor.
            int sockfd = get_sockfd(HOST, PORT);
            if(sockfd < 0){
                puts("\nSomething is fucky. Can't\nsend. Is server down?");
            }

            else{
                // Send packet.
                uint16_t p_size = packet_size + 2;
                uint16_t totalsent = 0;
                uint16_t bytesleft = p_size; 
                int n;

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
                    puts("\nPacket sent successfully.");
                        uint32_t receipt;
                        
                        n = recv(sockfd, &receipt, 4, 0);
                        if(n == -1){
                            perror("recv1");
                        }
                        else{
                            #if BYTE_ORDER == LITTLE_ENDIAN
                            receipt = ntohl(receipt);
                            #endif
                            printf("\njob_id received = %" PRIu32, receipt);
                            
                            // Put job_id into booking_buffer.
                            booking_buffer.job_id = receipt;
                            
                            // Append booking_buffer to the linked list.
                            append(&root, &curr, booking_buffer);
                        }
                }
                free(packet);
                close(sockfd);
            }
        }
        else if(ch == 2){
            uint32_t id;
            if(root == NULL)
                puts("\nEmpty list: nothing to delete.");
            else{
                printf("Enter job_id to delete: ");
                scanf("%" SCNu32, &id);
                getchar();
                delete(&root, &curr, id);
            }
        }
        else if(ch == 3){}
        else if(ch == 4){}
        else if(ch == 5){
            show_list(root);
        }
        else if(ch == 6){
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
        if((length = strlen(head->end)) > longest) longest = length;
        if((length = strlen(head->name)) > longest) longest = length;
        if((length = strlen(head->phone)) > longest) longest = length;
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
     