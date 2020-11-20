/* server.c - code for server program. Do not rename this file */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <poll.h>
#include <errno.h>
#include "trie.c"

//Constant for length and global word
#define QLEN 6

typedef struct participant_client{
    char* username;
    int sd;
    int connectedToObserver;
    int active;
} participant_client;

typedef struct observer_client{
    char* username;
    int sd;
    int active;
} observer_client;



participant_client participants[255];
observer_client observers[255];


int updateFD(fd_set fd, int sd, int max_sd){
    FD_SET(sd, &fd);
    if(max_sd < (sd+1)){
        return sd+1;
    }else{
        return max_sd;
    }
}

int participant_connect(int participant_sd){
    char response = 'Y';

    struct sockaddr cad;
    socklen_t alen = sizeof(cad);
    int sd = accept(participant_sd, &cad, &alen);
    if(sd < 0){
        fprintf(stderr,"Error: Accept failed");
        close(sd);
        return -1;
    }

    int enough_room = 0;
    int i = 0;
    for(; i < 255; i++){
        if(participants[i].active == 0){
            enough_room = 1;
            break;
        }
    }

    if(!enough_room){
        response = 'N';
        send(sd, &response, sizeof(char), MSG_NOSIGNAL);
        close(sd);
        return -1;
    }

    participants[i].sd = sd;
    participants[i].active = 1;

    send(sd, &response, sizeof(char), 0);

    struct pollfd fds;
    fds.fd = sd;
    fds.events = POLLIN;

    int timeout = 60;
    char username[255];

    while(1){
        int r = poll(&fds, 1, timeout*1000);
        if(r < 0 && errno != EINTR){
            fprintf(stderr,"Error: Poll failed");
            close(sd);
            return -1;
        }else if(r == 0){
            close(sd);
            return -1;
        }else if(fds.revents & POLLIN){
            r = recv(sd, username, strlen(username), 0);
            if(r < 0){
                fprintf(stderr,"Error: Recv failed");
                close(sd);
                return -1;
            }else if(r == 0){
                close(sd);
                return -1;
            }else{
                //CHECK IF NAME IS VALID
                //
                //THREE CASES:
                //1.) INVALID USERNAME 'I' -> LOOP w/ SAME TIME
                //2.) USERNAME TAKEN 'T' -> LOOP 
                //3.) VALID 'Y' -> BREAK OUT OF LOOP
            }
        }
    }

    username[strlen(username)] = '\0';
    participants[i].username = username;

    char message[27];
    snprintf(message, 26, "User %s has joined", username);

    //SEND MESSAGE TO ALL USERS 
    return 1;
}



int observer_connect(int observer_sd){
    char response = 'Y';

    struct sockaddr cad;
    socklen_t alen = sizeof(cad);
    int sd = accept(observer_sd, &cad, &alen);
    if(sd < 0){
        fprintf(stderr,"Error: Accept failed");
        close(sd);
        return -1;
    }

    int enough_room = 0;
    int i = 0;
    for(; i < 255; i++){
        if(observers[i].active == 0){
            enough_room = 1;
            break;
        }
    }

    if(!enough_room){
        response = 'N';
        send(sd, &response, sizeof(char), MSG_NOSIGNAL);
        close(sd);
        return -1;
    }

    observers[i].sd = sd;
    observers[i].active = 1;

    send(sd, &response, sizeof(char), 0);

    struct pollfd fds;
    fds.fd = sd;
    fds.events = POLLIN;

    int timeout = 60;
    char username[255];

    while(1){
        int r = poll(&fds, 1, timeout*1000);
        if(r < 0 && errno != EINTR){
            fprintf(stderr,"Error: Poll failed");
            close(sd);
            return -1;
        }else if(r == 0){
            close(sd);
            return -1;
        }else if(fds.revents & POLLIN){
            r = recv(sd, username, strlen(username), 0);
            if(r < 0){
                fprintf(stderr,"Error: Recv failed");
                close(sd);
                return -1;
            }else if(r == 0){
                close(sd);
                return -1;
            }else{
                //CHECK IF NAME IS VALID
                //
                //THREE CASES:
                //1.) INVALID USERNAME 'N' -> BREAK LOOP, FAILURE
                //2.) USERNAME TAKEN 'T' -> LOOP 
                //3.) VALID 'Y' -> BREAK OUT OF LOOP
            }
        }
    }

    char message[26];
    snprintf(message, 26, "A new observer has joined");

    //SEND MESSAGE TO ALL USERS 
    return 1;
}

//Checks to see if the given character is in the mystery word
int main(int argc, char* argv[]){

    //variables to store network info
    struct protoent *ptrp; 
    struct sockaddr_in observer_cad; 
    struct sockaddr_in participant_cad; 
    int observer_sd, participant_sd; 
    int alen; 
    int optval = 1;  
    uint8_t guesses;
    struct timeval tv;

    int max_sd;
    fd_set readfds, writefds;
    fd_set wrk_readfds, wrk_writefds;

    //Check # of arguments
    if(argc != 3){
        fprintf(stderr,"Error: Wrong number of arguments\n");
        fprintf(stderr,"usage:\n");
        fprintf(stderr,"./server participant_port observer_port\n");
        exit(EXIT_FAILURE);
    }

    //Check if guess # is valid
    uint8_t participant_port = atoi(argv[1]);
    uint8_t observer_port = atoi(argv[2]);

    //clear sockaddr struct and set family & Ip address
    memset((char *)&participant_cad,0,sizeof(participant_cad)); 
    memset((char *)&observer_cad,0,sizeof(observer_cad));

    participant_cad.sin_family = AF_INET; 
    participant_cad.sin_addr.s_addr = INADDR_ANY;
    observer_cad.sin_family = AF_INET; 
    observer_cad.sin_addr.s_addr = INADDR_ANY;

    
    if (participant_port > 0) { 
        participant_cad.sin_port = htons((u_short)participant_port); 
    } else {  
        fprintf(stderr,"Error: Bad port number %s\n",argv[1]); 
        exit(EXIT_FAILURE);
    } 
   
    if (observer_port > 0) { 
        observer_cad.sin_port = htons((u_short)observer_port); 
    } else {  
        fprintf(stderr,"Error: Bad port number %s\n",argv[2]); 
        exit(EXIT_FAILURE);
    } 

    if(participant_port == observer_port){
        fprintf(stderr,"Error: Bad port number %s\n",argv[2]); 
        exit(EXIT_FAILURE);
    }

    //Map TCP transport protocol
    if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) { 
        fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number"); 
        exit(EXIT_FAILURE);
    }

    //Create socket 
    participant_sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto); 
    if (participant_sd < 0) {
        fprintf(stderr, "Error: Socket creation failed\n"); 
        exit(EXIT_FAILURE); 
    } 

    observer_sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto); 
    if (observer_sd < 0) {
        fprintf(stderr, "Error: Socket creation failed\n"); 
        exit(EXIT_FAILURE); 
    } 

    //Set port to be reused
    if( setsockopt(participant_sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0 ) { 
        fprintf(stderr, "Error: Setting socket option failed\n"); 
        exit(EXIT_FAILURE); 
    } 

    if( setsockopt(observer_sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0 ) { 
        fprintf(stderr, "Error: Setting socket option failed\n"); 
        exit(EXIT_FAILURE); 
    } 

    //Bind local addr to socket
    if (bind(participant_sd, (struct sockaddr *)&participant_cad, sizeof(participant_cad)) < 0) { 
        fprintf(stderr,"Error: Bind failed\n"); 
        exit(EXIT_FAILURE); 
    } 
    
    if (bind(observer_sd, (struct sockaddr *)&observer_cad, sizeof(observer_cad)) < 0) { 
        fprintf(stderr,"Error: Bind failed\n"); 
        exit(EXIT_FAILURE); 
    } 

    //Set queue length
    if (listen(participant_sd, QLEN) < 0) { 
        fprintf(stderr,"Error: Listen failed\n"); 
        exit(EXIT_FAILURE); 
    } 

    if (listen(observer_sd, QLEN) < 0) { 
        fprintf(stderr,"Error: Listen failed\n"); 
        exit(EXIT_FAILURE); 
    } 

    
    FD_ZERO(&readfds);
    FD_SET(participant_sd, &readfds);
    FD_ZERO(&writefds);
    FD_SET(observer_sd, &writefds);
    max_sd = participant_sd;

    for(int i = 0; i < 255; i++){
        participants[i].connectedToObserver = 0;
        participants[i].active = 0;
        observers[i].active = 0;
    }
    
    //Main server loop
    int retval;
    while(1){

        FD_ZERO(&wrk_readfds);
        for (int i = 0; i < max_sd; ++i) {
            if (FD_ISSET(i, &readfds)) {
                FD_SET(i, &wrk_readfds);
            }
        }

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        retval = select(max_sd + 1, &wrk_readfds, NULL, NULL, &tv);
        if(retval == -1){
            fprintf(stderr,"Error: Read failed\n"); 
            exit(EXIT_FAILURE);
        }

        for(int i = 0; i < max_sd + 1; i++){

            if(FD_ISSET(i, &wrk_readfds)){

                if(i == participant_sd){
                    int code = participant_connect(i);
                    if(code == -1){
                        break;
                    }
                    max_sd = updateFD(readfds, i, max_sd);
                }else if(i == observer_sd){
                    int code = observer_connect(i);
                    if(code == -1){
                        break;
                    }
                    updateFD(writefds, i, max_sd);
                }

            }else{
                // FIND THE INDEX AND CHECK IF A MESSAGE IS COMING THROUGH
            }
        }
    }
}

