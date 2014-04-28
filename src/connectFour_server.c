#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#define CLIENT_NUM 2

/**
 * Opens a new TCP listening socket on an ephemeral port.
 * Returns an integer for the listening file descriptor,
 * or -1 on error.
 */
int open_listenfd(int);


int main(int argc, char** argv) {

    int listenfd, clientfds[CLIENT_NUM];
    struct sockaddr_in server, clients[CLIENT_NUM];
    socklen_t sock_size;

    // create a new TCP server on a random port and get that port number
    listenfd = open_listenfd(0);
    if (getsockname(listenfd, (struct sockaddr*)&server, &sock_size) < 0)
        return -1;



    exit(0);
}

int open_listenfd(int portnum) {

    int listenfd, optval=1;
    struct sockaddr_in serveraddr;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
            (const void *) &optval, sizeof(int)) < 0)
        return -1;

    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short) portnum);

    if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    if (listen(listenfd, 1024) < 0)
        return -1;

    return listenfd;
}