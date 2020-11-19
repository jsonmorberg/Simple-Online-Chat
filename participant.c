/* participant.c - code for participant. Do not rename this file */
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

void participant(int sd){

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
    printf("Choose a username: ");

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
                    printf("Username is already taken. Choose a different username: ");
                }else if(response == 'I'){
                    printf("Username is invalid. Choose a valid username: ");
                }

            }else{
                printf("Choose a username (upto 10 characters long; allowed characters are alphabets, digits, and underscores): ");
            }

        }else{
            printf("Time to choose a username has expired.\n");
            exit(EXIT_SUCCESS);
        }
    }

    free(buf)
    printf("Username accepted.");

    char message[10000];
    for(;;){

        int privateFlag = 0;

        printf("Enter message: ");
        scanf("%s", message);

        if(strlen(message) > 0){
            if(message[0] == '@'){
                privateFlag = 1;
            }

            if(strlen(message) > 1000){
                char fragment[1000];

                int index = 0;
                for(int i = 0; i < strlen(message); i++){

                    if(index == 0 && privateFlag == 1){
                        if(message[0] != '@'){
                            fragment[0] = '@';
                            index++; 
                        }

                    }else{
                        if(index != 1000){
                            fragment[index] = message[i];
                            index++;

                        }else if(i = strlen(message)){
                            uint16_t messageLength = htonl(strlen(fragment));
                            send(sd, &messageLength, sizeof(uint16_t), 0);
                            send(sd, fragment, strlen(fragment), 0);
                        }else{
                            index = 0;
                            i--;
                            uint16_t messageLength = htonl(strlen(fragment));
                            send(sd, &messageLength, sizeof(uint16_t), 0);
                            send(sd, fragment, strlen(fragment), 0);
                        }
                    }
                }
            }else{
                uint16_t messageLength = htonl(strlen(message));
                send(sd, &messageLength, sizeof(uint16_t), 0);
                send(sd, message, strlen(message), 0);
            }
        }
    }
}

int main(int argc, char **argv) {
    struct hostent *ptrh; /* pointer to a host table entry */
    struct protoent *ptrp; /* pointer to a protocol table entry */
    struct sockaddr_in sad; /* structure to hold an IP address */
    int sd; /* socket descriptor */
    int port; /* protocol port number */
    char *host; /* pointer to host name */

    struct pollfd mypoll = { STDIN_FILENO, POLLIN|POLLPRI }; 
    int timeout = 60 * 1000;



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

    participant(sd);
    close(sd);
    exit(EXIT_SUCCESS);
}
