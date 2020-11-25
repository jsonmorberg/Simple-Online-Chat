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
#include <time.h>
#include "trie.c"
#include <errno.h>

extern int errno ;

//Constant for length and global word
#define QLEN 6

typedef struct client{
    char* username;
    int sd;
    int connectedObserver;
    int observerIndex;
    int active;
    int hasUsername;
    time_t time;
} client;

client participants[255];
client observers[255];

char validateWord(char *word){
	int len = strlen(word);

	if (len < 1 || len > 10){
		return 'I';
	}

	for (int i = 0; i < len; i++){
		char letter = word[i];
		if (!isalpha(letter) && letter != '_' && !isdigit(letter)){
			return 'I';
		}
	}

    for(int i = 0; i < 255; i++){
        if(participants[i].active){
            if(strcmp(word, participants[i].username) == 0){
                return 'T';
            }
        }
    }

	return 'Y';
}

int receiveMessage(int index){
    uint16_t length;
    int retval = recv(participants[index].sd, &length, sizeof(uint16_t), MSG_WAITALL);
    length = ntohs(length);

    if(retval <= 0 || length > 1000){
        return 0;
    }
}

int participant_username(int index){

    uint8_t len;
    char username[255];
    if(recv(participants[index].sd, &len, sizeof(uint8_t), MSG_WAITALL) == 0){
        return 0;
    }

    if(recv(participants[index].sd, username, len, MSG_WAITALL) == 0){
        return 0;
    }
    username[len] = '\0';

    printf("Got Username: %s", username);

    char validation = validateWord(username);

    if(validation == 'Y'){
        send(participants[index].sd, &validation, sizeof(char), MSG_NOSIGNAL);
        participants[index].username = username;
        participants[index].hasUsername = 1;
        participants[index].time = 0;

    }else if(validation == 'T'){
        send(participants[index].sd, &validation, sizeof(char), MSG_NOSIGNAL);
        participants[index].time = time(NULL);

    }else if(validation == 'I'){
        send(participants[index].sd, &validation, sizeof(char), MSG_NOSIGNAL);
    }

    char connect_message[27];
    snprintf(connect_message, 26, "User %s has joined", username);
    connect_message[strlen(connect_message)] = '\0';
    //SEND TO ALL
    return 1;
}

int observer_username(int index){
    uint8_t len;
    char username[255];

    if(recv(observers[index].sd, &len, sizeof(uint8_t), MSG_WAITALL) == 0){
        return 0;
    }

    if(recv(observers[index].sd, username, len, MSG_WAITALL) == 0){
        return 0;
    }
    username[len] = '\0';

    printf("Got Username: %s", username);
    
    char validation = 'N';

    for(int i = 0; i < 255; i++){
        if(strcmp(participants[i].username, username) == 0){
            if(participants[i].connectedObserver){
                validation = 'T';
                break;
            }else{
                participants[i].observerIndex = index;
                participants[i].connectedObserver = 1;
                validation = 'Y';
                break;
            }
        }
    }

    if(validation == 'Y'){
        send(observers[index].sd, &validation, sizeof(char), MSG_NOSIGNAL);
        observers[index].username = username;
        observers[index].hasUsername = 1;
        observers[index].time = 0;
        
    }else if(validation == 'T'){
        send(observers[index].sd, &validation, sizeof(char), MSG_NOSIGNAL);
        time(&observers[index].time );

    }else if(validation == 'N'){
        send(observers[index].sd, &validation, sizeof(char), MSG_NOSIGNAL);
        observers[index].time = 0;
        observers[index].active = 0;
        return 0;
    }

    //char connect_message[26];
    //snprintf(connect_message, 25, "A new observer has joined");
    //connect_message[25] = '\0';
    //SEND TO ALL
    return 1;
}

int participant_connect(int participant_sd){
    char response = 'Y';

    struct sockaddr cad;
    socklen_t alen = sizeof(cad);
    int sd = accept(participant_sd, &cad, &alen);

    if(sd < 0){
        fprintf(stderr,"Error: Participant Accept failed");
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

    time(&participants[i].time);
    participants[i].sd = sd;
    participants[i].active = 1;

    send(sd, &response, sizeof(char), 0);
    return sd;
}

int observer_connect(int observer_sd){
    char response = 'Y';

    struct sockaddr cad;
    socklen_t alen = sizeof(cad);
    int sd = accept(observer_sd, &cad, &alen);

    if(sd < 0){
        fprintf(stderr,"Error: Observer Accept failed");
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

    time(&observers[i].time);
    observers[i].sd = sd;
    observers[i].active = 1;
    send(sd, &response, sizeof(char), 0);

    return sd;
}

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

    int max_fd;
    fd_set readfds;
    fd_set wrk_readfds;

    //Check # of arguments
    if(argc != 3){
        fprintf(stderr,"Error: Wrong number of arguments\n");
        fprintf(stderr,"usage:\n");
        fprintf(stderr,"./server participant_port observer_port\n");
        exit(EXIT_FAILURE);
    }

    //Check if guess # is valid
    int participant_port = atoi(argv[1]);
    int observer_port = atoi(argv[2]);

    //clear sockaddr struct and set family & Ip address
    memset((char *)&participant_cad,0,sizeof(participant_cad)); 
    memset((char *)&observer_cad,0,sizeof(observer_cad));

    participant_cad.sin_family = AF_INET; 
    participant_cad.sin_addr.s_addr = INADDR_ANY;
    observer_cad.sin_family = AF_INET; 
    observer_cad.sin_addr.s_addr = INADDR_ANY;

    if(participant_port == observer_port){
        fprintf(stderr,"Error: Don't use the same port for both participant and observer\n"); 
        exit(EXIT_FAILURE);
    }

    
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
        int errnum = errno;

        fprintf(stderr, "Error: Participant Bind failed\n");
        exit(EXIT_FAILURE); 
    } 
    
    if (bind(observer_sd, (struct sockaddr *)&observer_cad, sizeof(observer_cad)) < 0) { 
        fprintf(stderr,"Error: Observer Bind failed\n"); 
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
    FD_SET(observer_sd, &readfds);
    max_fd = participant_sd > observer_sd ? participant_sd : observer_sd;

    for(int i = 0; i < 255; i++){
        participants[i].connectedObserver = 0;
        participants[i].active = 0;
        participants[i].hasUsername = 0;
        participants[i].username = " ";
        observers[i].active = 0;
        observers[i].hasUsername = 0;
        observers[i].username = " ";
    }
    
    //Main server loop
    int retval;
    while(1){

        FD_ZERO(&wrk_readfds);
        memcpy(&wrk_readfds, &readfds, sizeof(fd_set));

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        retval = select(max_fd + 1, &wrk_readfds, NULL, NULL, &tv);
        if(retval == -1){
            fprintf(stderr,"Error: select(2) error"); 
            exit(EXIT_FAILURE);

        } else if (retval == 0){
            printf("TIME CHECK\n");
            for(int i = 0; i < 255; i++){

                time_t current;
                time(&current);
                
                if((participants[i].active) && (participants[i].time > 0)){
                    if(difftime(current, participants[i].time) >= 60){
                        printf("A participant timedout\n");
                        FD_CLR(participants[i].sd, &readfds);
                        close(participants[i].sd);
                        participants[i].sd = 0;
                        participants[i].time = 0;
                        participants[i].active = 0;
                    }
                }

                if((observers[i].active) && (observers[i].time > 0)){
                    if(difftime(current, observers[i].time) >= 60){
                        printf("An observer timedout\n");
                        FD_CLR(observers[i].sd, &readfds);
                        close(observers[i].sd);
                        observers[i].sd = 0;
                        observers[i].time = 0;
                        observers[i].active = 0;
                    }
                }
            }
        }

        if(FD_ISSET(participant_sd, &wrk_readfds)){
            int code = participant_connect(participant_sd);
            if(code != -1){
                FD_SET(code, &readfds);
                max_fd = code > max_fd ? code : max_fd;
            }
        }

        if(FD_ISSET(observer_sd, &wrk_readfds)){
            int code = observer_connect(observer_sd);
            if(code != -1){
                FD_SET(code, &readfds);
                max_fd = code > max_fd ? code : max_fd;
            }
        }

        //skip over the first 3 fds (stdin, stdout, stderr)
        for(int i = 8; i < max_fd + 1; i++){
            printf("sd: %d\n", i);

            //find index of existing participants/observers
            if(FD_ISSET(i, &wrk_readfds)){

                printf("Inside for loop foreal\n");
                int index;
                int participant_flag = 0;
                for(int j = 0; j < 255; j++){
                    if(participants[j].sd == i){
                        printf("Sd is a participant\n");
                        index = j;
                        participant_flag = 1;
                        break;

                    }else if(observers[j].sd == i){
                        printf("Sd is an observer\n");
                        index = j;
                        break;
                    }
                }

                if(participant_flag){
                    if(participants[index].hasUsername){
                        printf("oh cool u wanna send something\n");
                    }else{
                        //GET USERNAME
                        printf("PARTICIPANT USERNAME\n");
                        if(!participant_username(index)){
                            FD_CLR(participants[index].sd, &readfds);
                            close(participants[index].sd);
                            participants[index].sd = 0;
                        }
                    }
                }else{

                    if(observers[index].hasUsername){
                        printf("OBSERVER DISCONNECTED");
                        observers[index].username = " ";
                        observers[index].active = 0;
                        observers[index].hasUsername = 0;
                        FD_CLR(observers[index].sd, &readfds);
                        close(observers[index].sd);
                        observers[index].sd = 0;

                    }else{
                        //GET USERNAME
                        printf("OBSERVER USERNAME\n");
                        if(!observer_username(index)){
                            FD_CLR(observers[index].sd, &readfds);
                            close(observers[index].sd);
                            observers[index].sd = 0;
                        }
                    }
                }
            }
            
        }
    }
}

