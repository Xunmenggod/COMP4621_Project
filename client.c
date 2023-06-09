#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "chatroom.h"

#define MAX 1024 // max buffer size
#define PORT 6789  // port number
#define RETRY_LIMITS 5 // retry times for the password login

static int sockfd;
static uint8_t needClose = 0;

void generate_menu(){
	printf("Hello dear user pls select one of the following options:\n");
	printf("EXIT\t-\t Send exit message to server - unregister ourselves from server\n");
    printf("WHO\t-\t Send WHO message to the server - get the list of current users except ourselves\n");
    printf("#<user>: <msg>\t-\t Send <MSG>> message to the server for <user>\n");
    printf("Or input messages sending to everyone in the chatroom.\n");
}

void recv_server_msg_handler() {
    /********************************/
	/* receive message from the server and desplay on the screen*/
	/**********************************/
	char rec_buffer[MAX] = {0};
	while (1)
	{
		if (needClose)
			pthread_exit(NULL);
		else
		{
			bzero(rec_buffer, sizeof(rec_buffer));
			int recieveBytes = recv(sockfd, rec_buffer, sizeof(rec_buffer), 0);
			printf("%s\n", rec_buffer);
		}
	}
}

void read_keyboard(char* buffer, const int size)
{
	bzero(buffer, size);
	int n = 0;
	while ((buffer[n++] = getchar()) != '\n');
	buffer[n - 1] = '\0';
}

int main(){
    int n;
	int nbytes;
	struct sockaddr_in server_addr, client_addr;
	char buffer[MAX];
	
	/******************************************************/
	/* create the client socket and connect to the server */
	/******************************************************/
	sockfd = socket(AF_INET, SOCK_STREAM, 0); //construct the sockfd with the ipv4 and tcp
	if (sockfd == -1)
	{
		perror("client socket construction");
		exit(0); //@TODO: bonus pt for the interruption handling
	}else
		printf("Socket successfully created...\n");
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // the server address is the loopback addr
	server_addr.sin_port = htons(PORT);
	// connection
	if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
	{
		perror("connecting");
		exit(0);
	}else
		printf("Connected to the server...\n");

	generate_menu();
	// recieve welcome message to enter the nickname
    bzero(buffer, sizeof(buffer));
    if (nbytes = recv(sockfd, buffer, sizeof(buffer), 0)==-1){
        perror("recv");
    }
    printf("%s\n", buffer);

	/*************************************/
	/* Input the nickname and send a message to the server */
	/* Note that we concatenate "REGISTER" before the name to notify the server it is the register/login message*/
	/*******************************************/
	char user_name[C_NAME_LEN] = {0};
	char password[C_PASSWORD_LEN] = {0};
	int selection;
	n = 0;
	read_keyboard(buffer, sizeof(buffer));
	while (strcmp(buffer, "REGISTER") != 0 && strcmp(buffer, "LOG_IN") != 0)
	{
		printf("There is no such selection, please check your format and reselect\n");
		bzero(buffer, sizeof(buffer));
		read_keyboard(buffer, sizeof(buffer));
	}
	printf("Please first enter the nick name\n");
	read_keyboard(user_name, sizeof(user_name));
	if (strcmp(buffer, "REGISTER") == 0)
	{
		selection = 0;
	
		printf("Please set the password for your account\n");
	}else if (strcmp(buffer, "LOG_IN") == 0)
	{
		selection = 1;
		printf("Please enter the password for login\n");
	}
	read_keyboard(password, sizeof(password));

	// bzero(buffer, sizeof(buffer));
	// strcpy(buffer, "REGISTER ");
	strcat(buffer , " ");
	strcat(buffer, user_name);
	strcat(buffer, " ");
	strcat(buffer, password);
	nbytes = send(sockfd, buffer, sizeof(buffer), 0);
	if (nbytes < 1) 
	{
		perror("send");
		close(sockfd);
		exit(0);
	}

    // receive welcome message "welcome xx to joint the chatroom. A new account has been created." (registration case) or "welcome back! The message box contains:..." (login case)
    bzero(buffer, sizeof(buffer));
    if (recv(sockfd, buffer, sizeof(buffer), 0)==-1){
        perror("recv");
    }

	if (selection)
	{
		// log in error handling
		int verficationTimes = 0;
		while (strncmp(buffer, "Welcome", 7) != 0)
		{
			if (strncmp(buffer, "WRONG", 5) == 0)
			{
				verficationTimes++;
				if (verficationTimes == RETRY_LIMITS)
				{	
					int sleepTime = 10;
					verficationTimes = 0;
					printf("Wrong password for so many times, please re-enter the password after %ds\n", sleepTime);
					usleep(10 * 1e6);
					printf("Now, you can re-enter the password\n");
					continue;
				}else
					printf("%s\n", buffer);
				read_keyboard(password, sizeof(password));
			}else
			{
				printf("%s \n",buffer);
				printf("Please first re-enter the nickname\n");
				read_keyboard(user_name, sizeof(user_name));
				printf("Please then enter the password for the new nickname \n");
				read_keyboard(password, sizeof(password));
			}
			// send and recieve between client and server
			bzero(buffer, sizeof(buffer));
			strcat(buffer, "LOG_IN ");
			strcat(buffer, user_name);
			strcat(buffer, " ");
			strcat(buffer, password);
			nbytes = send(sockfd, buffer, sizeof(buffer), 0);
			if (nbytes < 1) 
			{
				perror("send");
				close(sockfd);
				exit(0);
			}
			bzero(buffer, sizeof(buffer));
			if (recv(sockfd, buffer, sizeof(buffer), 0)==-1){
				perror("recv");
			}
		}
	}else
	{
		// register error handling
		while(strncmp(buffer, "Welcome", 7) != 0)
		{
			printf("%s\n", buffer);
			printf("Please first re-enter the nickname\n");
			read_keyboard(user_name, sizeof(user_name));
			printf("Please then set the password for the new nickname \n");
			read_keyboard(password, sizeof(password));
			// send and recieve between client and server
			bzero(buffer, sizeof(buffer));
			strcat(buffer, "REGISTER ");
			strcat(buffer, user_name);
			strcat(buffer, " ");
			strcat(buffer, password);
			nbytes = send(sockfd, buffer, sizeof(buffer), 0);
			if (nbytes < 1) 
			{
				perror("send");
				close(sockfd);
				exit(0);
			}
			bzero(buffer, sizeof(buffer));
			if (recv(sockfd, buffer, sizeof(buffer), 0)==-1){
				perror("recv");
			}
		}
		printf("%s\n", buffer);
		bzero(buffer, sizeof(buffer));
		if (recv(sockfd, buffer, sizeof(buffer), 0)==-1){
			perror("recv");
		}
	}
    printf("%s\n", buffer);

    /*****************************************************/
	/* Create a thread to receive message from the server*/
	/* pthread_t recv_server_msg_thread;*/
	/*****************************************************/
	pthread_t rec_thread_handler;
	int ret_thread = pthread_create(&rec_thread_handler, NULL, recv_server_msg_handler, NULL);
	if (ret_thread != 0)
	{
		printf("Pthread creation failed with return value %d\n", ret_thread);
		exit(0);
	}

	// chat with the server
	for (;;) {
		bzero(buffer, sizeof(buffer));
		n = 0;
		while ((buffer[n++] = getchar()) != '\n')
			;
		if ((strncmp(buffer, "EXIT", 4)) == 0) {
			printf("Client Exit...\n");
			/********************************************/
			/* Send exit message to the server and exit */
			/* Remember to terminate the thread and close the socket */
			/********************************************/
			nbytes = send(sockfd, buffer, sizeof(buffer), 0);
			if (nbytes < 1)
			{
				perror("send");
				exit(0);
			}else
				printf("EXIT message sent to server\n");
			needClose = 1;
			if (close(sockfd) == -1)
			{
				perror("close");
				exit(0);
			}else
				printf("It's OK to close the window Now OR enter ctrl+c");
		}
		else if (strncmp(buffer, "WHO", 3) == 0) {
			printf("Getting user list, pls hold on...\n");
			if (send(sockfd, buffer, sizeof(buffer), 0)<0){
				puts("Sending MSG_WHO failed");
				exit(1);
			}
			printf("If you want to send a message to one of the users, pls send with the format: '#username:message'\n");
		}
		else if (strncmp(buffer, "#", 1) == 0) {
			// If the user want to send a direct message to another user, e.g., aa wants to send direct message "Hello" to bb, aa needs to input "#bb:Hello"
			if (send(sockfd, buffer, sizeof(buffer), 0)<0){
				printf("Sending direct message failed...\n");
				exit(1);
			}
		}
		else {
			/*************************************/
			/* Sending broadcast message. The send message should be of the format "username: message"*/
			/**************************************/
			char prefix[MAX] = {0};
			strcpy(prefix, user_name);
			strcat(prefix, ": ");
			strcat(prefix, buffer);
			nbytes = send(sockfd, prefix, sizeof(prefix), 0);
			if (nbytes < 1)
			{
				perror("send");
				exit(0);
			}
		}
	}
	return 0;
}

