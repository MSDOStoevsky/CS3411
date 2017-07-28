#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define h_addr h_addr_list[0]

/**********************************/
/* Author: Dylan Lettinga         */
/* Date: 08/04/2017               */
/**********************************/
int main(int argc, char* argv[])
{
    int sockfd, portno, arg;
    char buffer[1000];
    char* sarg;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    /* reject an execution with no arguments */
    if(argv[1] == NULL || argv[2] == NULL || argv[3] == NULL)
    {
        write(STDERR_FILENO, "Usage: reclient <addr> <port> <cmd> <args>\n", 43);
        exit(EXIT_FAILURE);
    }

    server = gethostbyname(argv[1]);
    portno = atoi(argv[2]);
    sarg = argv[3];

    /* stringify arguments sent to server */
    for (arg = 4; arg < argc; arg++)
    {
        sprintf(sarg, "%s %s", sarg, argv[arg]);
    }
    
    /* Create a socket point */
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        write(STDERR_FILENO, "socket error: accept\n", 21);
        exit(EXIT_FAILURE);
    }

    if (server == NULL) {
        write(STDERR_FILENO, "socket error: open\n", 19);
        exit(EXIT_FAILURE);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        write(STDERR_FILENO, "failed to connect\n", 18);
        exit(EXIT_FAILURE);
    }

    /* send commands */
    if(write(sockfd, sarg, strlen(sarg)) < 0){
        write(STDERR_FILENO, "ERROR writing to socket\n", 24);
        exit(EXIT_FAILURE);
    }
    bzero(buffer,1000);
    
    /* read result into buffer */
    if(read(sockfd, buffer, 1000) < 0){
        write(STDERR_FILENO, "ERROR reading from socket\n", 26);
        exit(EXIT_FAILURE);
    }

    /* print result */
    write(STDOUT_FILENO, buffer, sizeof(buffer));

    return 0;
}