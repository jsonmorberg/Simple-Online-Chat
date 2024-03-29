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
#include <errno.h>

//Constant for length and global word
#define QLEN 6

//struct for participant
typedef struct participant_client{
    char username[11];
    int sd;
    int connectedToObserver;
    int observerIndex;
    int active;
    int hasUsername;
    time_t time;
} participant_client;

//struct for observer
typedef struct observer_client{
    char username[11];
    int sd;
    int connectedToParticipant;
    int participantIndex;
    int active;
    int hasUsername;
    time_t time;
} observer_client;

//main arrays that will keep the clients
participant_client participants[255];
observer_client observers[255];

//check if the username is valid and return the proper char
char validateWord(char *word){
	int len = strlen(word);

    //word between 1 and 10
	if (len < 1 || len > 10){
		return 'I';
	}

    //alphanumeric
	for (int i = 0; i < len; i++){
		char letter = word[i];
		if (!isalpha(letter) && letter != '_' && !isdigit(letter)){
			return 'I';
		}
	}

    //not used by other participants 
    for(int i = 0; i < 255; i++){
        if(participants[i].active){
            if(strcmp(word, participants[i].username) == 0){
                return 'T';
            }
        }
    }

	return 'Y';
}

//send to each observer 
void sendAll(char * message, int length){
    uint16_t nlength = htons(length);
    for(int i = 0; i < 255; i++){
        if(observers[i].active){
            send(observers[i].sd, &nlength, sizeof(uint16_t), MSG_NOSIGNAL);
            send(observers[i].sd, message, length, MSG_NOSIGNAL);
        }
    }
}

//reset an observer struct
void resetObserver(int i){
    close(observers[i].sd);
    observers[i].active = 0;
    observers[i].time = 0;
    observers[i].sd = 0;

    if(observers[i].hasUsername){
        memset(observers[i].username, '\0', 11);
        observers[i].hasUsername = 0;
    }

    if(observers[i].connectedToParticipant){
        int participantIndex = observers[i].participantIndex;
        participants[participantIndex].connectedToObserver = 0;
        participants[participantIndex].observerIndex = 0;

        observers[i].connectedToParticipant = 0;
        observers[i].participantIndex = 0;
    }
}

//reset a participant struct
void resetParticipant(int i){
    close(participants[i].sd);
    participants[i].active = 0;
    participants[i].time = 0;
    participants[i].sd = 0;

    if(participants[i].hasUsername){

        //alert observers that a participant left
        char message[25];
        snprintf(message, 24, "User %s has left", participants[i].username);
        message[25] = '\0';
        sendAll(message, strlen(message));
        
        memset(participants[i].username, '\0', 11);
        participants[i].hasUsername = 0;
    }

    if(participants[i].connectedToObserver){
        resetObserver(participants[i].observerIndex);
        participants[i].connectedToObserver = 0;
        participants[i].observerIndex = 0;
    }
}

//process an incoming message
int receiveMessage(int index){
    uint16_t length;
    int retval = recv(participants[index].sd, &length, sizeof(uint16_t), MSG_WAITALL);
    length = ntohs(length);

    if(retval <= 0 || length > 1000){
        return 0;
    }

    char message[length+14];

    //pad zeros & append username
    int zeros = 11 - strlen(participants[index].username);
    int i = 0;
    for(; i <= zeros; i++){
        message[i] = ' ';
    }

    snprintf(message+i, strlen(participants[index].username) + 3, "%s: ", participants[index].username);

    if(recv(participants[index].sd, message+14, length, MSG_WAITALL) == 0){
        return 0;
    }
    message[length + 14] = '\0';

    //check for a non whitespace character
	int hasNonSpace = 0;
	for (int j = 0; j < length; j++){
		if (!isspace(message[j + 14])){
			hasNonSpace = 1;
		}
	}

    if(!hasNonSpace){
        return 0;
    }

    if(message[14] == '@'){
        //private message
        message[0] = '%';
        
        //get the destinaton username
        char username[11];
        char* ptr = &message[15];

        i = 0;
        while(*ptr != ' '){
            username[i] = *ptr;
            ptr++;
            i++;
        }
        username[i] = '\0';
        
        //find the observer tied to destination username
        int foundFlag = 0;
        for(i = 0; i < 255; i++){
            if(observers[i].active){
                if(strcmp(username, observers[i].username) == 0){
                    foundFlag = 1;
                    break;
                }
            }
        }

        if(foundFlag){
            
            //send the private message to destination observer
            uint16_t responseLength = htons(strlen(message));
            int observersIndex = participants[index].observerIndex;
            send(observers[observersIndex].sd, &responseLength, sizeof(uint16_t), MSG_NOSIGNAL);
            send(observers[observersIndex].sd, message, strlen(message), MSG_NOSIGNAL);

            //send the private message to current observer if attached
            if(participants[index].connectedToObserver){
                send(participants[index].sd, &responseLength, sizeof(uint16_t), MSG_NOSIGNAL);
                send(participants[index].sd, message, strlen(message), MSG_NOSIGNAL);
            }

            return 1;

        }else{

            if(participants[index].connectedToObserver){

                //between 31 and 41 length, +1 for \0
                char response[42];
                snprintf(response, 41, "Warning: user %s doesn't exist...", username);
                response[42] = '\0';
            
                //send to current observer
                uint16_t responseLength = htons(strlen(response));
                int observersIndex = participants[index].observerIndex;
                send(observers[observersIndex].sd, &responseLength, sizeof(uint16_t), MSG_NOSIGNAL);
                send(observers[observersIndex].sd, response, strlen(response), MSG_NOSIGNAL); 
            }
            return 1;
        }

    }else{
        //public message

        message[0] = '>';
        sendAll(message, strlen(message));
        return 1;
    }
}

//get the username for a participant
int participant_username(int index){

    //get username
    uint8_t len;
    char username[255];
    int retval = recv(participants[index].sd, &len, sizeof(uint8_t), MSG_WAITALL);
    if(retval == 0){
        return 0;
    }

    if(recv(participants[index].sd, username, len, MSG_WAITALL) == 0){
        return 0;
    }
    username[len] = '\0';
    
    //run through a validator
    char validation = validateWord(username);

    if(validation == 'Y'){
        send(participants[index].sd, &validation, sizeof(char), MSG_NOSIGNAL);
        strncpy(participants[index].username, username, len+1);
        participants[index].hasUsername = 1;
        participants[index].time = 0;

    }else if(validation == 'T'){
        send(participants[index].sd, &validation, sizeof(char), MSG_NOSIGNAL);
        participants[index].time = time(NULL);
        return 1;

    }else if(validation == 'I'){
        send(participants[index].sd, &validation, sizeof(char), MSG_NOSIGNAL);
        return 1;
    }

    char connect_message[27];
    snprintf(connect_message, 26, "User %s has joined", username);
    connect_message[27] = '\0';

    sendAll(connect_message, strlen(connect_message));
    return 1;
}

//get username for an observer
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
    
    //default case
    char validation = 'N';

    //search for participant w/ username
    for(int i = 0; i < 255; i++){
        if(strcmp(participants[i].username, username) == 0){
            if(participants[i].connectedToObserver){
                validation = 'T';
                break;
            }else{
                participants[i].observerIndex = index;
                participants[i].connectedToObserver = 1;
                observers[index].participantIndex = i;
                observers[index].connectedToParticipant = 1;
                validation = 'Y';
                break;
            }
        }
    }

    //handle validation 
    if(validation == 'Y'){
        send(observers[index].sd, &validation, sizeof(char), MSG_NOSIGNAL);
        strncpy(observers[index].username, username, len+1);
        observers[index].hasUsername = 1;
        observers[index].time = 0;
        
    }else if(validation == 'T'){
        send(observers[index].sd, &validation, sizeof(char), MSG_NOSIGNAL);
        time(&observers[index].time);
        return 1;

    }else if(validation == 'N'){
        send(observers[index].sd, &validation, sizeof(char), MSG_NOSIGNAL);
        observers[index].time = 0;
        observers[index].active = 0;
        return 0;
    }

    char connect_message[27];
    snprintf(connect_message, 26, "A new observer has joined");
    connect_message[27] = '\0';
    
    sendAll(connect_message, strlen(connect_message));
    return 1;
}

//connect a participant
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

//connect an observer
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
    int optval = 1;  
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

    //set ports and validate
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

    //set select fds    
    FD_ZERO(&readfds);
    FD_SET(participant_sd, &readfds);
    FD_SET(observer_sd, &readfds);
    max_fd = participant_sd > observer_sd ? participant_sd : observer_sd;

    //initialize structs for clients
    for(int i = 0; i < 255; i++){
        participants[i].connectedToObserver = 0;
        participants[i].active = 0;
        participants[i].hasUsername = 0;
        memset(participants[i].username, '\0', 11);
        observers[i].active = 0;
        observers[i].hasUsername = 0;
        memset(observers[i].username, '\0', 11);
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
            for(int i = 0; i < 255; i++){

                time_t current;
                time(&current);
                
                //participant timeout check
                if((participants[i].active) && (participants[i].time > 0)){
                    if(difftime(current, participants[i].time) >= 60){
                        FD_CLR(participants[i].sd, &readfds);
                        resetParticipant(i);
                    }
                }

                //observer timeout check
                if((observers[i].active) && (observers[i].time > 0)){
                    if(difftime(current, observers[i].time) >= 60){
                        FD_CLR(observers[i].sd, &readfds);
                        resetObserver(i);
                    }
                }
            }
        }

        //participant connection
        if(FD_ISSET(participant_sd, &wrk_readfds)){
            int code = participant_connect(participant_sd);
            if(code != -1){
                FD_SET(code, &readfds);
                max_fd = code > max_fd ? code : max_fd;
            }
        }

        //participant connection
        if(FD_ISSET(observer_sd, &wrk_readfds)){
            int code = observer_connect(observer_sd);
            if(code != -1){
                FD_SET(code, &readfds);
                max_fd = code > max_fd ? code : max_fd;
            }
        }

        //skip over the first 3 fds (stdin, stdout, stderr)
        for(int i = 3; i < max_fd + 1; i++){

            //find index of existing participants/observers
            if(FD_ISSET(i, &wrk_readfds)){
                int index;
                int participant_flag = 0;
                for(int j = 0; j < 255; j++){
                    if(participants[j].sd == i){
                        index = j;
                        participant_flag = 1;
                        break;

                    }else if(observers[j].sd == i){
                        index = j;
                        break;
                    }
                }

                if(participant_flag){
                    if(participants[index].hasUsername){

                        //incoming message
                        if(!receiveMessage(index)){
                            if(participants[index].connectedToObserver){
                                FD_CLR(observers[participants[index].observerIndex].sd, &readfds);
                            }
                            FD_CLR(participants[index].sd, &readfds);
                            resetParticipant(index); 
                        }
                    }else{

                        //participant needs a username
                        if(!participant_username(index)){
                            FD_CLR(participants[index].sd, &readfds);
                            resetParticipant(index);
                        }
                    }
                }else{
                    
                    //observer has disconnected
                    if(observers[index].hasUsername){
                        FD_CLR(observers[index].sd, &readfds);
                        resetObserver(index);

                    }else{
                        
                        //observer needs a username
                        if(!observer_username(index)){
                            FD_CLR(observers[index].sd, &readfds);
                            resetObserver(index);
                        }
                    }
                }
            } 
        }
    }
}

