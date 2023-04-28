/**
 *  chat.h - Header file for network communications/threads exercise
 *
 *  part of a chat system
 */

#include <netinet/in.h>

/*
 *  Maximum size of display name
 */
#define C_NAME_LEN 	24
#define C_PASSWORD_LEN 40


/*
* Used to store user information (nickname and corresponding server socket FD)
*/
typedef enum USER_STATE
{
    OFFLINE = 0,
    ONLINE = 1
} STATE;

typedef struct user_info
{
    int sockfd; // socket fd
    char username[C_NAME_LEN + 1]; 
    char password[C_PASSWORD_LEN + 1]; // this is for the storage of user password
    int state; // online or offline. For example, online=1 or offline=0.
} user_info_t;

