/* echo_server.c - code for example server program that uses TCP */
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#define QLEN 6 /* size of request queue */
int visits = 0; /* counts client connections */

/*------------------------------------------------------------------------
* Program: echo_server
*
* Purpose: allocate a socket and then repeatedly execute the following:
* (1) wait for the next connection from a client
* (2) send a short message to the client
* (3) close the connection
* (4) go back to step (1)
*
* Syntax: ./echo_server port
*
* port - protocol port number to use
*
*------------------------------------------------------------------------
*/

int main(int argc, char **argv) {
    struct protoent *ptrp; /* pointer to a protocol table entry */
    struct sockaddr_in sad; /* structure to hold server's address */
    struct sockaddr_in cad; /* structure to hold client's address */
    int sd, sd2; /* socket descriptors */
    int port; /* protocol port number */
    int alen; /* length of address */
    int optval = 1; /* boolean value when we set socket option */
    char buf[1000]; /* buffer for string the server sends */

    int max_sd;
    fd_set readfds, writefds;
    fd_set wrk_readfds, wrk_writefds;

    struct timeval tv;

    if (argc != 2) {
        fprintf(stderr, "Error: Wrong number of arguments\n");
        fprintf(stderr, "usage:\n");
        fprintf(stderr, "./server server_port\n");
        exit(EXIT_FAILURE);
    }

    memset((char *) &sad, 0, sizeof(sad)); /* clear sockaddr structure */
    sad.sin_family = AF_INET; /* set family to Internet */
    sad.sin_addr.s_addr = INADDR_ANY; /* set the local IP address */


    port = atoi(argv[1]); /* convert argument to binary */
    if (port > 0) { /* test for illegal value */
        sad.sin_port = htons((u_short) port);
    } else { /* print error message and exit */
        fprintf(stderr, "Error: Bad port number %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    /* Map TCP transport protocol name to protocol number */
    if (((long int) (ptrp = getprotobyname("tcp"))) == 0) {
        fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
        exit(EXIT_FAILURE);
    }

    /* Create a socket */
    sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
    if (sd < 0) {
        fprintf(stderr, "Error: Socket creation failed\n");
        exit(EXIT_FAILURE);
    }


    /* Allow reuse of port - avoid "Bind failed" issues */
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        fprintf(stderr, "Error Setting socket option failed\n");
        exit(EXIT_FAILURE);
    }

    /* Bind a local address to the socket */
    if (bind(sd, (struct sockaddr *) &sad, sizeof(sad)) < 0) {
        fprintf(stderr, "Error: Bind failed\n");
        exit(EXIT_FAILURE);
    }

    /* Specify size of request queue */
    if (listen(sd, QLEN) < 0) {
        fprintf(stderr, "Error: Listen failed\n");
        exit(EXIT_FAILURE);
    }

    FD_ZERO(&readfds);
    FD_SET(sd, &readfds);

    FD_ZERO(&writefds);

    max_sd = sd + 1 ;

    /* Main server loop - accept and handle requests */
    int ret;
    while (1) {

        // Initial the set of addresses with sockets that you want to monitor to know when they are ready to recv/accept.
        // select() modifies the set of addresses given to it, so we need to reset that set again.
        FD_ZERO(&wrk_readfds);
        for (int i = 0; i < max_sd; ++i) {
            if (FD_ISSET(i, &readfds)) {
                FD_SET(i, &wrk_readfds);
            }
        }


        tv.tv_sec = 1;
        tv.tv_usec = 0;

        ret = select(max_sd + 1, &wrk_readfds, NULL, NULL, &tv);
        if (ret == -1) {
            fprintf(stderr, "Error: select() failed. %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        } else if (ret == 0) {
            // nothing interesting happened on any of the sockets.
            continue;
        }
        if (FD_ISSET(sd, &wrk_readfds)) {
            alen = sizeof(cad);
            if ((sd2 = accept(sd, (struct sockaddr *) &cad, &alen)) < 0) {
                fprintf(stderr, "Error: Accept failed\n");
                exit(EXIT_FAILURE);
            }
            visits++;
            sprintf(buf, "This server has been contacted %d time%s\n", visits, visits == 1 ? "." : "s.");
            send(sd2, buf, strlen(buf), 0);
            close(sd2);
        }
    }
}

