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
#include <ctype.h>

int checkWord(char *word){
    int len = strlen(word);
    
    if(len < 1 || len > 10){
        return 0;
    } 

    for(int i = 0; i < len; i++){
        char letter = word[i];
        if(!isalpha(letter) && letter != '_' && !isdigit(letter)){
            return 0;
        }
    }    
    
    return 1;
}

void participant(int sd){

    struct pollfd mypoll = { STDIN_FILENO, POLLIN|POLLPRI }; 
    int timeout = 60 * 1000;

    char response;
    if(recv(sd, &response, sizeof(char), 0) == 0){
        return;
    }

    if(response == 'N'){
        printf("Server is full");
        return;
    }

    char buf[100];
    printf("Choose a username: ");
    fflush(stdout);

    while(1){
        if(poll(&mypoll, 1, timeout)){

            scanf("%s", buf);
            
            if(checkWord(buf)){

                uint8_t wordLength = strlen(buf);
                buf[wordLength] = ' ';
                send(sd, &wordLength, sizeof(uint8_t), 0);
                send(sd, buf, wordLength, 0);

                if(recv(sd, &response, sizeof(char), 0) == 0){
                    return;
                }

                if(response == 'Y'){
                    break;
                }else if(response == 'T'){
                    printf("Username is already taken. Choose a different username: ");
                    fflush(stdout);
                }else if(response == 'I'){
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

    printf("Username accepted.\n");

    char message[10000];
    for(;;){

        int privateFlag = 0;

        printf("Enter message: ");
        fflush(stdout);
        scanf("%s", message);

        if(strlen(message) > 1000){
            //fragment message into sizes of 1000 MAX

            char fragment[1000];
            uint16_t j = 0;

            for(int i = 0; i < strlen(message); i++){
                fragment[j] = message[i];
                
                
                if(j == 999){
                    uint16_t fragLength = htons(j+1);
                    send(sd, &fragLength, sizeof(uint16_t), MSG_NOSIGNAL);
                    send(sd, fragment, strlen(message), MSG_NOSIGNAL);
                    j = 0;
                }else{
                    j++;
                }
            }

            if(j != 0){
                uint16_t fragLength = htons(j+1);
                send(sd, &fragLength, sizeof(uint16_t), MSG_NOSIGNAL);
                send(sd, fragment, strlen(message), MSG_NOSIGNAL);
            }

        }else{
            uint16_t messageLength = htons(strlen(message));
            send(sd, &messageLength, sizeof(uint16_t), MSG_NOSIGNAL);
            send(sd, message, strlen(message), MSG_NOSIGNAL);
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
