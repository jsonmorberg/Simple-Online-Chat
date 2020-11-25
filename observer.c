/*observer.c - code for observer. Do not rename this file */ 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <poll.h>

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

void observer(int sd){

	fd_set readfds;
	fd_set wrk_readfds;
	struct timeval tv;
	int max_fd;

	FD_ZERO(&readfds);
    FD_SET(0, &readfds);
    FD_SET(sd, &readfds);
    max_fd = sd;

	int timeout = 60 * 1000;

	char response;
	recv(sd, &response, sizeof(char), 0);

	if (response == 'N'){
		printf("Server is full\n");
		return;
	}

	printf("Enter a participant's username: ");
	fflush(stdout);

	int retval;
	while (1){

		FD_ZERO(&wrk_readfds);
        memcpy(&wrk_readfds, &readfds, sizeof(fd_set));

		tv.tv_sec = 60;
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
				uint8_t wordLength = strlen(buf);
				buf[wordLength] = ' ';
				send(sd, &wordLength, sizeof(uint8_t), MSG_NOSIGNAL);
				send(sd, buf, wordLength, MSG_NOSIGNAL);

				if (recv(sd, &response, sizeof(char), 0) == 0){
					return;
				}

				if (response == 'Y'){
					break;
				}else if (response == 'T'){
					printf("That participant already has an observer. Enter a different participant's username: ");
					fflush(stdout);
				}else if (response == 'N'){
					printf("There is no active participant with that username.\n");
					return;
				}
			}else{
				printf("Enter a participant's username: ");
				fflush(stdout);
			}
		}else{
			printf("Time to enter a username has expired.\n");
			return;
		}
	}

	for (;;){
		uint16_t messageLength = 0;
		if (recv(sd, &messageLength, sizeof(uint16_t), MSG_WAITALL) == 0){
			return;
		}

		messageLength = ntohs(messageLength);

		char message[messageLength];
		if (recv(sd, message, messageLength, MSG_WAITALL) == 0){
			return;
		}

		message[messageLength] = '\0';
		printf("%s\n", message);
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

	observer(sd);
	close(sd);
	exit(EXIT_SUCCESS);
}