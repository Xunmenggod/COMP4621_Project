#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "chatroom.h"
#include <poll.h>

#define MAX 1024 // max buffer size
#define PORT 6789 // server port number
#define MAX_USERS 50 // max number of users
static unsigned int users_count = 0; // number of registered users

static user_info_t *listOfUsers[MAX_USERS] = {NULL}; // list of users


/* Add user to userList */
void user_add(const user_info_t *user);
/* Get user name from userList */
char * get_username(const int sockfd);
/* Get user sockfd by name */
int get_sockfd(const char *name);

/* Add user to userList */
void user_add(const user_info_t *user){
	if(users_count ==  MAX_USERS){
		printf("sorry the system is full, please try again later\n");
		return;
	}
	/***************************/
	/* add the user to the list */
	/**************************/
	// listOfUsers[users_count]->password = user->password;
	listOfUsers[users_count] = malloc(sizeof(user_info_t));
	strcpy(listOfUsers[users_count]->username, user->username);
	listOfUsers[users_count]->sockfd = user->sockfd;
	listOfUsers[users_count]->state = user->state;
	users_count += 1;
}

/* Determine whether the user has been registered  */
//return value: 1 means this is new user based on the name, 0 means the name is already registered
int isNewUser(const char* name) {
	int i;
	int flag = 1;
	/*******************************************/
	/* Compare the name with existing usernames */
	/*******************************************/
	for (i = 0; i < MAX_USERS; i++)
	{
		if(listOfUsers[i] == NULL)
			break;
		if (strcmp(listOfUsers[i]->username, name) == 0)
		{
			flag = 0; // find the registered user -> set the flag to be 0
			break;
		}
	}
	return flag;
}

/* Get user name from userList */
char * get_username(const int ss){
	int i;
	static char uname[MAX];
	/*******************************************/
	/* Get the user name by the user's sock fd */
	/*******************************************/
	for (i = 0; i < MAX_USERS; i++)
	{
		if(listOfUsers[i] == NULL)
			break;
		if (listOfUsers[i]->state == OFFLINE)
			continue;
		if (listOfUsers[i]->sockfd == ss)
		{	
			strcpy(uname, listOfUsers[i]->username);
			break;
		}
	}
	printf("get user name: %s\n", uname);
	return uname;
}

/* Get user sockfd by name */
int get_sockfd(const char *name){
	int i;
	int sock;
	/*******************************************/
	/* Get the user sockfd by the user name */
	/*******************************************/
	for (i = 0; i < MAX_USERS; i++)
	{
		if(listOfUsers[i] == NULL)
			break;
		if (strcmp(listOfUsers[i]->username, name) == 0)
			break;
	}
	sock = listOfUsers[i]->sockfd;
	return sock;
}

// The following two functions are defined for poll()
// Add a new file descriptor to the set
void add_to_pfds(struct pollfd* pfds[], int newfd, int* fd_count, int* fd_size)
{
	// If we don't have room, add more space in the pfds array
	if (*fd_count == *fd_size) {
		*fd_size *= 2; // Double it

		*pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
	}

	(*pfds)[*fd_count].fd = newfd;
	(*pfds)[*fd_count].events = POLLIN; // Check ready-to-read

	(*fd_count)++;
}

// Remove an index from the set
void del_from_pfds(struct pollfd pfds[], int i, int* fd_count)
{
	// Copy the one from the end over this one
	pfds[i] = pfds[*fd_count - 1];

	(*fd_count)--;
}

typedef enum RECV_TARGTS
{
	ALL = 0,
	OTHERS = 1
} TARGETS;

int broadcast_online(const char* message, const int size, const int sending_client_fd, int targets)
{
	int i, result;
	for (i = 0; i < MAX_USERS; i++)
	{
		if (listOfUsers[i] == NULL)
			break;
		if (listOfUsers[i]->sockfd == sending_client_fd && targets == OTHERS)
			continue;
		else
		{
			if (listOfUsers[i]->state == ONLINE)
			{
				int result = send(listOfUsers[i]->sockfd, message, size,  0);
				if (result < 1)
				{
					perror("broadcasting messages");
					return 0;
				}
			}
		}
	}
	return result; // if no error, the return value should be larger than 0, 0 indicates error
}

int broadcast(const char* message, const int size, const int sending_client_fd, int targets)
{
	int i, result;
	for (i = 0; i < MAX_USERS; i++)
	{
		if (listOfUsers[i] == NULL)
			break;
		if (listOfUsers[i]->sockfd == sending_client_fd && targets == OTHERS)
			continue;
		else
		{
			if (listOfUsers[i]->state == ONLINE)
			{
				int result = send(listOfUsers[i]->sockfd, message, size,  0);
				if (result < 1)
				{
					perror("broadcasting messages");
					return 0;
				}
			}else
			{ 
				// handle the offline user
				char user_file[C_NAME_LEN] = {0};
				strcpy(user_file, listOfUsers[i]->username);
				strcat(user_file, ".txt");
				FILE* fp = fopen(user_file, "a");
				if (fp == NULL)
				{
					printf("Could not find the user message box for %s \n", listOfUsers[i]->username);
					return 0;
				}else
				{
					fseek(fp, 0, SEEK_END);
					char file_message[MAX] = {0};
					// char* name = get_username(sending_client_fd);
					// strcat(name, ": ");
					// strcpy(file_message, name);
					strcat(file_message, message);
					// strcat(file_message, "\n");
					result = fprintf(fp, file_message);
					fclose(fp);
				}
			}
		}
	}
	return result; // if no error, the return value should be larger than 0, 0 indicates error
}


int main(){
	int listener;     // listening socket descriptor
	int newfd;        // newly accept()ed socket descriptor
	int addr_size;     // length of client addr
	struct sockaddr_in server_addr, client_addr;
	
	char buffer[MAX]; // buffer for client data
	int nbytes;
	int fd_count = 0;
	int fd_size = 5;
	struct pollfd* pfds = malloc(sizeof(*pfds) * fd_size);
	
	int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, u, rv;
    
	/**********************************************************/
	/*create the listener socket and bind it with server_addr*/
	/**********************************************************/
	listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener == -1)
	{
		perror("listener construction");
		exit(0);
	}else
		printf("Socket successfully created..\n");
	// use the setsockopt to reuse the port
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
		perror("ste reusable address socket failed");
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);
	if (bind(listener, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
	{
		perror("Server socket binding");
		exit(0);
	}else
		printf("Socket successfully binded..\n");

	// Now server is ready to listen and verification
	if ((listen(listener, 5)) != 0) {
		printf("Listen failed...\n");
		exit(3);
	}
	else
		printf("Server listening..\n");
		
	// Add the listener to set
	pfds[0].fd = listener;
	pfds[0].events = POLLIN; // Report ready to read on incoming connection
	fd_count = 1; // For the listener
	
	// main loop
	for(;;) {
		/***************************************/
		/* use poll function */
		/**************************************/
		int poll_count = poll(pfds, fd_count, -1);
		if (poll_count == -1)
		{
			perror("poll");
			exit(1);
		}

		// run through the existing connections looking for data to read
		for(i = 0; i < fd_count; i++) {
			if (pfds[i].revents & POLLIN) { // we got one!!
				if (pfds[i].fd == listener) {
					/**************************/
					/* we are the listener and we need to handle new connections from clients */
					/****************************/
					addr_size = sizeof(client_addr);
					newfd = accept(pfds[i].fd, (struct sockaddr*)&client_addr, &addr_size);		
					if (newfd == -1)
						perror("accept");
					else
						add_to_pfds(&pfds, newfd, &fd_count, &fd_size);

					// send welcome message
					bzero(buffer, sizeof(buffer));
					strcpy(buffer, "Welcome to the chat room!\nPlease enter a nickname.");
					if (send(newfd, buffer, sizeof(buffer), 0) == -1)
						perror("send");

				} else {
					// handle data from a client
					bzero(buffer, sizeof(buffer));
					if ((nbytes = recv(pfds[i].fd, buffer, sizeof(buffer), 0)) <= 0) {
						// got error or connection closed by client
						if (nbytes == 0) {
						// connection closed
						printf("pollserver: socket %d hung up\n", pfds[i].fd);
						} else {
						perror("recv");
						}
						close(pfds[i].fd); // Bye!
						del_from_pfds(pfds, i, &fd_count);
					} else {
						// we got some data from a client
						if (strncmp(buffer, "REGISTER", 8)==0){
							printf("Got register/login message\n");
							/********************************/
							/* Get the user name and add the user to the userlist*/
							/**********************************/
							char name[C_NAME_LEN] = {0};
							const char* delim = " ";
							char* token = strtok(buffer, delim);
							while (token != NULL)
							{
								token = strtok(NULL, delim);
								if (token != NULL)
									strcpy(name, token);
							} 
							char user_file[C_NAME_LEN] = {0};
							strcpy(user_file, name);
							strcat(user_file, ".txt");

							if (isNewUser(name) == 1) {
								/********************************/
								/* it is a new user and we need to handle the registration*/
								/**********************************/
								user_info_t user;
								user.sockfd = pfds[i].fd;
								user.state = ONLINE;
								strcpy(user.username, name);
								user_add(&user);
								printf("add user name: %s\n", name);
								/********************************/
								/* create message box (e.g., a text file) for the new user */
								/**********************************/
								FILE* fp;
								fp = fopen(user_file, "w"); // create <name>.txt file to store the offline messages
								fclose(fp);
								// broadcast the welcome message (send to everyone except the listener)
								bzero(buffer, sizeof(buffer));
								strcpy(buffer, "Welcome ");
								strcat(buffer, name);
								strcat(buffer, " to join the chat room!");

								/*****************************/
								/* Broadcast the welcome message*/
								/*****************************/
								broadcast_online(buffer, sizeof(buffer), pfds[i].fd, ALL);

								/*****************************/
								/* send registration success message to the new user*/
								/*****************************/
								bzero(buffer, sizeof(buffer));
								strcpy(buffer, "A new account has been created.");
								nbytes = send(pfds[i].fd, buffer, sizeof(buffer), 0);
								if (nbytes < 1)
									perror("server sending");

							} else {
								/********************************/
								/* it's an existing user and we need to handle the login. Note the state of user,*/
								/**********************************/
								for (j = 0; j < MAX_USERS; j++)
								{
									if (listOfUsers[j] == NULL)
										break;
									if (strcmp(name, listOfUsers[j]->username) == 0)
									{
										listOfUsers[j]->state = ONLINE;
										// update the newly connected client sockfd for the existing user.
										listOfUsers[j]->sockfd = pfds[i].fd;
										break;
									}
								}

								/********************************/
								/* send the offline messages to the user and empty the message box*/
								/**********************************/
								bzero(buffer, sizeof(buffer));
								strcat(buffer, "Welcome back! The message box contains: ");
								nbytes = send(pfds[i].fd, buffer, sizeof(buffer), 0);
								if (nbytes < 1)
									perror("sending");
								FILE* fp = fopen(user_file, "r");
								FILE* temp = fopen("temp.txt", "w");
								char offline_message[MAX];
								while (fgets(offline_message, MAX, fp) != NULL)
								{
									int n = 0;
									while (offline_message[n++] != '\n');
									offline_message[n - 1] = '\0';
									nbytes = send(pfds[i].fd, offline_message, sizeof(offline_message), 0);
									if (nbytes < 1)
										perror("sedning offline message");
									else
										printf("offline message: %s\n", offline_message);
								}
								fclose(fp);
								fclose(temp);
								remove(user_file);
								rename("temp.txt", user_file);

								// broadcast the welcome message (send to everyone except the listener)
								bzero(buffer, sizeof(buffer));
								strcat(buffer, name);
								strcat(buffer, " is online!");
								/*****************************/
								/* Broadcast the welcome message*/
								/*****************************/
								broadcast_online(buffer, sizeof(buffer), pfds[i].fd, OTHERS);
							}
						} else if (strncmp(buffer, "EXIT", 4)==0){
							printf("Got exit message. Removing user from system\n");
							// send leave message to the other members
							bzero(buffer, sizeof(buffer));
							strcpy(buffer, get_username(pfds[i].fd));
							strcat(buffer, " has left the chatroom");
							/*********************************/
							/* Broadcast the leave message to the other users in the group*/
							/**********************************/
							broadcast_online(buffer, sizeof(buffer), pfds[i].fd, OTHERS);

							/*********************************/
							/* Change the state of this user to offline*/
							/**********************************/
							for (j = 0; j < MAX_USERS; j++)
							{
								if (listOfUsers[j] == NULL)
									break;
								if (listOfUsers[j]->state == OFFLINE)
									continue;
								if (listOfUsers[j]->sockfd == pfds[i].fd)
								{
									listOfUsers[j]->state = OFFLINE;
									// for debug
									printf("Exit user name:%s \t state:%d\n", listOfUsers[j]->username, listOfUsers[j]->state);
									break;
								}
							}
							
							//close the socket and remove the socket from pfds[]
							close(pfds[i].fd);
							del_from_pfds(pfds, i, &fd_count);
						} else if (strncmp(buffer, "WHO", 3)==0) {
							// concatenate all the user names except the sender into a char array
							printf("Got WHO message from client.\n");
							char ToClient[MAX];
							bzero(ToClient, sizeof(ToClient));
							/***************************************/
							/* Concatenate all the user names into the tab-separated char ToClient and send it to the requesting client*/
							/* The state of each user (online or offline)should be labelled.*/
							/***************************************/
							for (j = 0; j < MAX_USERS; j++)
							{
								if (listOfUsers[j] == NULL)
									break;
								if (strcmp(get_username(pfds[i].fd),listOfUsers[j]->username) == 0)
									continue;
								strcat(ToClient, listOfUsers[j]->username);
								if (listOfUsers[j]->state == ONLINE)
									strcat(ToClient, "*");
								strcat(ToClient, "\t");
							}
							strcat(ToClient, "\n* means this user online");
							nbytes = send(pfds[i].fd, ToClient, MAX, 0);
							if (nbytes < 1)
								perror("sending");

						} else if (strncmp(buffer, "#", 1)==0){
							// send direct message 
							// get send user name:
							printf("Got direct message.\n");
							// get which client sends the message
							char sendname[MAX];
							// get the destination username
							char destname[MAX];
							// get dest sock
							int destsock;
							// get the message
							char msg[MAX];
							/**************************************/
							/* Get the source name xx, the target username and its sockfd*/
							/*************************************/
							char* source_name = get_username(pfds[i].fd);
							strcpy(sendname, source_name);
							const char* symbol = ":";
							char* target_name = strtok(&buffer[1], symbol);
							strcpy(destname, target_name);
							char* sendingMsg = strtok(NULL, symbol);
							strcpy(msg, sendingMsg);
							int n = 0;
							while (msg[n++] != '\n');
							msg[n - 1] = '\0';

							if (isNewUser(destname)) {
								/**************************************/
								/* The target user is not found. Send "no such user..." messsge back to the source client*/
								/*************************************/
								bzero(buffer, sizeof(buffer));
								strcpy(buffer, "There is no such user. Please check your input format.");
								nbytes = send(pfds[i].fd, buffer, sizeof(buffer), 0);
								if (nbytes < 1)
									perror("sending");
							} else {
								// The target user exists.
								// concatenate the message in the form "xx to you: msg"
								char sendmsg[MAX];
								strcpy(sendmsg, sendname);
								strcat(sendmsg, " to you: ");
								strcat(sendmsg, msg);

								/**************************************/
								/* According to the state of target user, send the msg to online user or write the msg into offline user's message box*/
								/* For the offline case, send "...Leaving message successfully" message to the source client*/
								/*************************************/
								for (j = 0; j < MAX_USERS; j++)
								{
									if (listOfUsers[j] == NULL)
										break;
									if (strcmp(destname, listOfUsers[j]->username) == 0)
									{
										if (listOfUsers[j]->state == ONLINE)
										{
											nbytes = send(listOfUsers[j]->sockfd, sendmsg, sizeof(sendmsg), 0);
											if (nbytes < 1)
												perror("sending");
										}else
										{
											// handle the offline user
											char user_file[C_NAME_LEN] = {0};
											strcpy(user_file, destname);
											strcat(user_file, ".txt");
											FILE* fp = fopen(user_file, "a");
											if (fp == NULL)
											{
												printf("Could not find the user message box for %s\n", destname);
											}else
											{
												fseek(fp, 0, SEEK_END);
												strcat(sendmsg, "\n");
												if (fprintf(fp, sendmsg) < 1)
													perror("message box appending");
												fclose(fp);
											}
											// for sender
											bzero(buffer, sizeof(buffer));
											strcat(buffer, destname);
											strcat(buffer, " is offline. Leaving message successfully");
											nbytes = send(pfds[i].fd, buffer, sizeof(buffer), 0);
											if (nbytes < 1)
												perror("sending");
										}
									}
								}
							}
						} else {
							printf("Got broadcast message from user\n");
							/*********************************************/
							/* Broadcast the message to all users except the one who sent the message*/
							/*********************************************/
							// for debug
							printf("broadcasting name:%s \t broadcasting message:%s", get_username(pfds[i].fd), buffer);
							int n = 0;
							while(buffer[n++] != '\n');
							buffer[n - 1] = '\0';
							broadcast(buffer, sizeof(buffer), pfds[i].fd, OTHERS);
							
						}   

					}
				} // end handle data from client
			} // end got new incoming connection
		} // end looping through file descriptors
	} // end for(;;) 

	return 0;
}
