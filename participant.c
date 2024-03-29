/*participant.c - code for participant. Do not rename this file */ 
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <ctype.h>
#include <time.h>

int checkWord(char *word){
	int len = strlen(word);

	if (len < 1 || len > 10){
		return 0;
	}

	for (int i = 0; i < len; i++){
		char letter = word[i];
		if (!isalpha(letter) && letter != '_' && !isdigit(letter)){
			return 0;
		}
	}

	return 1;
}

//Prompts user for a valid username and sends messages to the chatroom server
void participant(int sd){

    //select values
	fd_set readfds;
	fd_set wrk_readfds;
	struct timeval tv;
	int max_fd;

	FD_ZERO(&readfds);
    FD_SET(0, &readfds);
    FD_SET(sd, &readfds);
    max_fd = sd;

    //Check if server is full
	char response;
	if (recv(sd, &response, sizeof(char), 0) == 0){
		return;
	}

	if (response == 'N'){
		printf("Server is full");
		return;
	}

    //Prompt for username w/ error checking
	printf("Choose a username: ");
	fflush(stdout);
	int timer = 60;
	time_t start;
	time_t end;

	int retval;
	while (1){

		time(&start);
        FD_ZERO(&wrk_readfds);
        memcpy(&wrk_readfds, &readfds, sizeof(fd_set));

		tv.tv_sec = timer;
        tv.tv_usec = 0;
		retval = select(max_fd + 1, &wrk_readfds, NULL, NULL, &tv);

		if (retval != 0){

			if(FD_ISSET(sd, &wrk_readfds)){
				printf("Time to enter a username has expired.\n");
				return;
			}

			char *buf = NULL;
			size_t length = 0;
			getline(&buf, &length, stdin);

			if ((strlen(buf) > 0) && (buf[strlen(buf) - 1] == '\n')){
				buf[strlen(buf) - 1] = '\0';
			}

			if (checkWord(buf)){

                //send word to server and get response
				uint8_t wordLength = strlen(buf);
				buf[wordLength] = ' ';
				send(sd, &wordLength, sizeof(uint8_t), MSG_NOSIGNAL);
				send(sd, buf, wordLength, MSG_NOSIGNAL);

				if (recv(sd, &response, sizeof(char), 0) == 0){
					return;
				}

				if (response == 'Y'){
					break;
				}
				else if (response == 'T'){
					timer = 60;
					printf("Username is already taken. Choose a different username: ");
					fflush(stdout);
				}
				else if (response == 'I'){
					time(&end);
					timer =- (int)difftime(end, start);
					printf("Username is invalid. Choose a valid username: ");
					fflush(stdout);
				}
			}else{
				printf("Choose a username (upto 10 characters long; allowed characters are alphabets, digits, and underscores); ");
				fflush(stdout);
			}
		}else{
			printf("Time to choose a username has expired.\n");
			exit(EXIT_SUCCESS);
		}
	}

    //sending mode
	printf("Username accepted.\n");
	for (;;){
		int privateFlag = 0;
		int validMessage = 1;

		printf("Enter message: ");
		fflush(stdout);

		//select will monitor for server disconnection
		FD_ZERO(&wrk_readfds);
        memcpy(&wrk_readfds, &readfds, sizeof(fd_set));

		select(max_fd + 1, &wrk_readfds, NULL, NULL, NULL);

		if(FD_ISSET(sd, &wrk_readfds)){
			printf("Time to enter a username has expired.\n");
			return;
		}

		char *message = NULL;
		size_t length = 0;
		getline(&message, &length, stdin);

        //remove the appended newline character
		if ((strlen(message) > 0) && (message[strlen(message) - 1] == '\n')){
			message[strlen(message) - 1] = '\0';
		}

		char user[10];
		if (message[0] == '@' && message[1] != ' '){
            //private message

            //set flag and extract username 
            //incase of message fragmentation
			privateFlag = 1;
			int i = 0;
			for (; i < 10; i++){
				if (message[i] != ' ' && message[i] != '\0'){
					user[i] = message[i];
				}else{
					break;
				}
			}
			user[i] = '\0';

            //check for whitespace in message
			int hasSpace = 0;
			for (int j = 0; j < strlen(message); j++){
				if (isspace(message[j])){
					hasSpace = 1;
				}
			}

			if (!hasSpace){
				validMessage = 0;
			}
        }else{
            //public message

            //check for a non whitespace character
			int hasNonSpace = 0;
			for (int i = 0; i < strlen(message); i++){
				if (!isspace(message[i])){
					hasNonSpace = 1;
				}
			}

			if (!hasNonSpace){
				validMessage = 0;
			}
		}

		if (validMessage){
			if (strlen(message) > 1000){
				//message needs to be fragmented

				char fragment[1000];
				uint16_t j = 0;

                //if private message, start iterating past the username
				int i = 0;
				if (privateFlag){
					i = strlen(user) + 1;
				}

                //iterate through the message
				for (; i < strlen(message); i++){

                    //beginning of a private fragment, add @username
					if (j == 0 && privateFlag == 1){
						for (int k = 0; k < strlen(user); k++){
							fragment[k] = user[k];
						}

						fragment[strlen(user)] = ' ';
						j = strlen(user) + 1;
					}

                    //copy over message to fragment
					fragment[j] = message[i];

					if (j == 999){
						uint16_t fragLength = htons(j + 1);
						send(sd, &fragLength, sizeof(uint16_t), MSG_NOSIGNAL);
						send(sd, fragment, j + 1, MSG_NOSIGNAL);
						j = 0;

					}else{
						j++;
					}
				}

                //residue fragment sent
				if (j != 0){
					uint16_t fragLength = htons(j);
					send(sd, &fragLength, sizeof(uint16_t), MSG_NOSIGNAL);
					send(sd, fragment, j, MSG_NOSIGNAL);
				}

			}else{
				uint16_t messageLength = htons(strlen(message));
				send(sd, &messageLength, sizeof(uint16_t), MSG_NOSIGNAL);
				send(sd, message, strlen(message), MSG_NOSIGNAL);
			}
		}
	}
}

int main(int argc, char **argv){
	struct hostent * ptrh; /*pointer to a host table entry */
	struct protoent * ptrp; /*pointer to a protocol table entry */
	struct sockaddr_in sad; /*structure to hold an IP address */
	int sd; /*socket descriptor */
	int port; /*protocol port number */
	char *host; /*pointer to host name */

	memset((char*) &sad, 0, sizeof(sad)); /*clear sockaddr structure */
	sad.sin_family = AF_INET; /*set family to Internet */

	if (argc != 3){
		fprintf(stderr, "Error: Wrong number of arguments\n");
		fprintf(stderr, "usage:\n");
		fprintf(stderr, "./client server_address server_port\n");
		exit(EXIT_FAILURE);
	}

	port = atoi(argv[2]); /*convert to binary */
	if (port > 0) /*test for legal value */
		sad.sin_port = htons((u_short) port);
	else{
		fprintf(stderr, "Error: bad port number %s\n", argv[2]);
		exit(EXIT_FAILURE);
	}

	host = argv[1]; /*if host argument specified */

	/*Convert host name to equivalent IP address and copy to sad. */
	ptrh = gethostbyname(host);
	if (ptrh == NULL){
		fprintf(stderr, "Error: Invalid host: %s\n", host);
		exit(EXIT_FAILURE);
	}

	memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);

	/*Map TCP transport protocol name to protocol number. */
	if (((long int)(ptrp = getprotobyname("tcp"))) == 0){
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/*Create a socket. */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0){
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/*Connect the socket to the specified server. */
	if (connect(sd, (struct sockaddr *) &sad, sizeof(sad)) < 0){
		fprintf(stderr, "connect failed\n");
		exit(EXIT_FAILURE);
	}

	participant(sd);
	close(sd);
	exit(EXIT_SUCCESS);
}