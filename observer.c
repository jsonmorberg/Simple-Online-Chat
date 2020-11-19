/* observer.c - code for observer. Do not rename this file */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

int checkWord(char* word){

}

void observer(int sd){

    struct pollfd mypoll = { STDIN_FILENO, POLLIN|POLLPRI }; 
    int timeout = 60 * 1000;

    char response;
    recv(sd, &response, sizeof(char), 0);

    if(response == 'N'){
        printf("Server is full");
        return;
    }

    int flag = 0;
    char buf[100];
    printf("Enter a participant's username: ");

    while(flag){
        if(poll(&mypoll, 1, timeout)){

            scanf("%s", buf);
            if(checkWord(buf)){
                uint8_t wordLength = strlen(buf);
                send(sd, &wordLength, sizeof(uint8_t), 0);
                send(sd, buf, wordLength, 0);

                recv(sd, &response, sizeof(char), 0);
                if(response == 'Y'){
                    flag = 1;
                }else if(response == 'T'){
                    printf("That participant already has an observer. Enter a different participant's username: ");
                }else if(response == 'N'){
                    printf("There is no active participant with that username.\n");
                    exit(EXIT_SUCCESS);
                }

            }else{
               printf("Enter a participant's username: ");
            }

        }else{
            printf("Time to enter a username has expired.\n");
            exit(EXIT_SUCCESS);
        }
    }

    for(;;){

        uint16_t messageLength;
        recv(sd, &messageLength, sizeof(uint16_t), 0);


        messageLength = ntohl(messageLength);
        char message[messageLength + 1];
        message[messageLength] = '\0';
        recv(sd, message, messageLength, 0);
        printf("%s\n", message);
    }
}

int main(int argc, char **argv) {
    struct hostent *ptrh; /* pointer to a host table entry */
    struct protoent *ptrp; /* pointer to a protocol table entry */
    struct sockaddr_in sad; /* structure to hold an IP address */
    int sd; /* socket descriptor */
    int port; /* protocol port number */
    char *host; /* pointer to host name */

    



    memset((char *) &sad, 0, sizeof(sad)); /* clear sockaddr structure */
    sad.sin_family = AF_INET; /* set family to Internet */


    if (argc != 3) {
        fprintf(stderr, "Error: Wrong number of arguments\n");
        fprintf(stderr, "usage:\n");
        fprintf(stderr, "./client server_address server_port\n");
        exit(EXIT_FAILURE);
    }

    port = atoi(argv[2]); /* convert to binary */
    if (port > 0) /* test for legal value */
        sad.sin_port = htons((u_short) port);
    else {
        fprintf(stderr, "Error: bad port number %s\n", argv[2]);
        exit(EXIT_FAILURE);
    }

    host = argv[1]; /* if host argument specified */

    /* Convert host name to equivalent IP address and copy to sad. */
    ptrh = gethostbyname(host);
    if (ptrh == NULL) {
        fprintf(stderr, "Error: Invalid host: %s\n", host);
        exit(EXIT_FAILURE);
    }

    memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);

    /* Map TCP transport protocol name to protocol number. */
    if (((long int) (ptrp = getprotobyname("tcp"))) == 0) {
        fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
        exit(EXIT_FAILURE);
    }

    /* Create a socket. */
    sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
    if (sd < 0) {
        fprintf(stderr, "Error: Socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    /* Connect the socket to the specified server. */
    if (connect(sd, (struct sockaddr *) &sad, sizeof(sad)) < 0) {
        fprintf(stderr, "connect failed\n");
        exit(EXIT_FAILURE);
    }

    observer(sd);
    close(sd);
    exit(EXIT_SUCCESS);
}
