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
#define BUFFERSIZE 128
#define IBUFFERSIZE 182

/**********************************/
/* Author: Dylan Lettinga         */
/* Date: 08/10/2017               */
/**********************************/
int main(int argc, char* argv[])
{
    pid_t pid, wpid;
    int sockfd, portno, status;
    char* nick;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[BUFFERSIZE];
    char ibuffer[IBUFFERSIZE];
    bzero(buffer,BUFFERSIZE);
    bzero(ibuffer,IBUFFERSIZE);

    /* reject an execution with no arguments */
    if(argv[1] == NULL || argv[2] == NULL || argv[3] == NULL)
    {
        write(STDERR_FILENO, "Usage: reclient <addr> <port> <nickname>\n", 41);
        exit(EXIT_FAILURE);
    }

    server = gethostbyname(argv[1]);
    portno = atoi(argv[2]);
    nick = argv[3];
    
    /* Create a socket point */
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        write(STDERR_FILENO, "socket error: create\n", 21);
        exit(EXIT_FAILURE);
    }

    if (server == NULL) {
        write(STDERR_FILENO, "socket error: no server\n", 24);
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

    /* send conection information */
    if(write(sockfd, nick, strlen(nick)) < 0){
        write(STDERR_FILENO, "ERROR writing to socket\n", 24);
        exit(EXIT_FAILURE);
    }
    bzero(buffer,BUFFERSIZE);

    pid = fork();
    if (pid == 0) {
        /* send chat messages */
        while(1)
        {   
            /* read from keyboard into buffer */
            if(read(STDIN_FILENO, buffer, BUFFERSIZE) < 0){
                write(STDERR_FILENO, "ERROR reading from STDIN\n", 25);
                exit(EXIT_FAILURE);
            }
            /* write to server */
            if(write(sockfd, buffer, BUFFERSIZE) < 0){
                write(STDERR_FILENO, "ERROR writing to socket\n", 24);
                exit(EXIT_FAILURE);
            }
            bzero(buffer,BUFFERSIZE);
        }
        exit(EXIT_SUCCESS);
    } else if (pid < 0) {
        write(STDERR_FILENO, "fork error\n", 11);
        exit(EXIT_FAILURE);
    } else {
        /* read other client chat messages */
        while(read(sockfd, ibuffer, IBUFFERSIZE) > 0)
        {
            write(STDOUT_FILENO, ibuffer, IBUFFERSIZE);
        }
        write(STDERR_FILENO, "Connection to the server has been lost.\n", 40);
        /* be parental */
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 0;
}