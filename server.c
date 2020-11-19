/* server.c - code for server program. Do not rename this file */
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

//Constant for length and global word
#define QLEN 5
char* word;


//Checks to see if the given character is in the mystery word
int main(int argc, char* argv[]){

    //variables to store network info
    struct protoent *ptrp; 
    struct sockaddr_in sad; 
    struct sockaddr_in cad; 
    int sd, sd2; 
    int port;
    int alen; 
    int optval = 1;  
    uint8_t guesses;

    //Check # of arguments
    if(argc != 3){
        fprintf(stderr,"Error: Wrong number of arguments\n");
        fprintf(stderr,"usage:\n");
        fprintf(stderr,"./server server_port guesses secret_word\n");
        exit(EXIT_FAILURE);
    }

    //Check if guess # is valid
    uint8_t port2 = atoi(argv[2]);

    //Check if the word is proper length
   
    //clear sockaddr struct and set family & Ip address
    memset((char *)&sad,0,sizeof(sad)); 
    sad.sin_family = AF_INET; 
    sad.sin_addr.s_addr = INADDR_ANY;

    //Check to see if port # is valid
    port = atoi(argv[1]); 
    if (port > 0) { 
        sad.sin_port = htons((u_short)port); 
    } else {  
        fprintf(stderr,"Error: Bad port number %s\n",argv[1]); 
        exit(EXIT_FAILURE);
    } 

    //Map TCP transport protocol
    if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) { 
        fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number"); 
        exit(EXIT_FAILURE);
    }

    //Create socket 
    sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto); 
    if (sd < 0) {
        fprintf(stderr, "Error: Socket creation failed\n"); 
        exit(EXIT_FAILURE); 
    } 

    //Set port to be reused
    if( setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0 ) { 
        fprintf(stderr, "Error: Setting socket option failed\n"); 
        exit(EXIT_FAILURE); 
    } 

    //Bind local addr to socket
    if (bind(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) { 
        fprintf(stderr,"Error: Bind failed\n"); 
        exit(EXIT_FAILURE); 
    } 

    //Set queue length
    if (listen(sd, QLEN) < 0) { 
        fprintf(stderr,"Error: Listen failed\n"); 
        exit(EXIT_FAILURE); 
    } 

    //Main server loop
    while(1){

        alen = sizeof(cad);

        //Accept connection
        if ((sd2=accept(sd, (struct sockaddr *)&cad, &alen)) < 0) { 
            fprintf(stderr, "Error: Accept failed\n"); 
            exit(EXIT_FAILURE); 
        }
        //=======================================
        //server full or not
        //TODO add yes or no logic
        char openSpace = 'Y';
        send(sd2,&openSpace, sizeof('Y'), 0);
        //=======================================
        //check username logic
        uint8_t userLength;
        char  username[11];
        recv(sd2, &userLength, sizeof(uint8_t), 0);
        recv(sd2, username, userLength, 0);
        username[userLength] = '\0';
        printf("%s\n",username);
        char userValid = 'Y';
        send(sd2, &userValid, sizeof('Y'), 0);
        printf("here\n");
        close(sd2); 
    }
}

