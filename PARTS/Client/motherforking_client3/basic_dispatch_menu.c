/*
    An ugly retarded basic-as-fuck command line menu 
    to drive some of my dispatch functions.
    
    All bookings will be sent to the server.
    Server will forward these to anyone else
    logged in as dispatch.

    If a dispatcher logs in, they will get a copy.
    If server dies, it will use its own backup.
    
    ,--------------------------------------------------------.
    | TASK                                            STATUS |
    |--------------------------------------------------------|
    | * Finish the delete function.                 | DONE   |
    | * Fix the mallocing on the add function.      | DONE   |
    | * Try sending it again.                       |        |
    | * Put all linked list stuff in another file.  |        |
    | * Make string buffers fit perfectly.          | DONE   |
    | * Insert error checking everywhere.           |        |
    | * Apply const liberally.                      |        |
    | * Split booking function into a booking and   |        |
    |   'append' function.                          |        |
    | * Put constant limits in a header somewhere.  |        |
    | * See if I can make the if-else thing better. |        |
    | * See if unicode is necessary.                |        |
    | * Check to see if my functions should return  |        |
    |   something. Errors or something.             |        |
    | * Consider using tree instead of list.        |        |
    | * Check to make sure that parameters in       |        |
    |   malloc calls can't/don't overflow.          |        |
    '--------------------------------------------------------'
*/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "../../Include/booking_structs.h"

void show_menu(void);
int get_choice(void);                                   // Return menu choice: 1-6 only.
void make_booking(struct _booking** curr);              // Create and append booking to list.
void show_list(struct _booking* curr);                  // Display list.
void delete(struct _booking** rt);                      // Delete booking in list.
uint16_t fgets_no_newline(char *str, uint16_t size);    // Get string, strip newline, return length.
void get_string(char **str, uint16_t size);             // Get string, malloc for it, put it in str.

uint32_t job_id = 0;                                    // A counter to number jobs as they are 
                                                        // added. This will be server's job but 
                                                        // for testing, I'm using this.
                        
int main(void){
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
    puts("\n,--------------.");
    puts("| BOOKING MENU |");
    puts("|--------------|");
    puts("| 1 | ADD      |");
    puts("| 2 | DELETE   |");
    puts("| 3 | DISPATCH |");
    puts("| 4 | EDIT     |");
    puts("| 5 | SHOW     |");
    puts("| 6 | QUIT     |");
    puts("'--------------'");
    printf("\n: ");
}



void make_booking(struct _booking** curr){
    
    uint16_t size = 1024;
    
    if(((*curr) = malloc(sizeof(struct _booking))) == NULL){
        puts("malloc fail.");
        exit(1);
    }
    
    (*curr)->struct_id = 3;      // Identify the struct type to rebuild it later.
    (*curr)->job_id = job_id++;  // Give booking a number. Server will do this later.
    
    /* Note to my future self. Get a load of this pointer work
       below! Apparently, I hate my future self, but it works so
       that's all that matters (almost). Sorry dude! 2017/3/30 */
    
    printf("\n\nlocation start: ");
    get_string(&((*curr)->location.start), size);   
    
    printf("\nlocation end: ");
    get_string(&((*curr)->location.end), size);
    
    printf("\nname: ");
    get_string(&((*curr)->name), size);
    
    printf("\nphone #: ");
    get_string(&((*curr)->phone), size);
    
     printf("\nemail: ");
    get_string(&((*curr)->email), size);
    
    printf("\ninfo: ");
    get_string(&((*curr)->info), size);
    
    printf("\ntime: ");
    get_string(&((*curr)->time), size);
    
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
        printf("start: %s", curr->location.start);
        printf("end: %s", curr->location.end);
        printf("name: %s", curr->name);
        printf("phone: %s", curr->phone);
        printf("email: %s", curr->email);
        printf("info: %s", curr->info);
        printf("time: %s", curr->time);
        
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


void get_string(char **str, uint16_t size){
    char buffer[size];
    uint16_t str_len, i;
    
    // Read a string into the buffer, strip 
    // newline, and get its length.
    str_len = fgets_no_newline(buffer, size);

    // Make perfect size in str.
    *str = malloc(str_len + 1);
    
    // Copy buffer into str.
    i = 0;
    while(buffer[i] != '\0') {
        (*str)[i] = buffer[i];
        i++;
    }
    (*str)[i] == '\0';  

}


uint16_t fgets_no_newline(char *str, uint16_t size){
    
    // Get string, might include newline.
    fgets(str, size, stdin);
    
    // Length of string including any newline.
    uint16_t str_len = strlen(str);
    
    // If empty string, we're done.
    if(str_len == 0) return 0;
    
    // Replace newline with null terminator.
    str_len--;
    if(str[str_len] == '\n'){
        str[str_len] == '\0';
        return str_len;
    }
    else return str_len + 1;
}



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
    
}
