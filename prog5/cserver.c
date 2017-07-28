#define _BSD_SOURCE
#define _SVID_SOURCE
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#define BUFFERSIZE 128
#define OUTBUFFERSIZE 180

/**********************************/
/* Author: Dylan Lettinga         */
/* Date: 08/10/2017               */
/**********************************/
struct Client {
    char nick[53];
    int endpoint;
};
void addcli(struct Client *list, struct Client cli, int num);
int main()
{
    pid_t pid, wpid;
    socklen_t clilen;
    int sockfd, newsockfd;
    int status, length;
    int *numcli;
    
    struct Client cli;
    struct sockaddr_in serv_addr, cli_addr;
    struct Client *cli_list = malloc(sizeof(cli));

    char buffer[BUFFERSIZE];
    char outbuff[OUTBUFFERSIZE];
    bzero(buffer,BUFFERSIZE);
    bzero(outbuff,OUTBUFFERSIZE);

    numcli = mmap(NULL, sizeof *numcli, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *numcli = 0;

    /* create socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        write(STDERR_FILENO, "socket error: open\n", 19);
        exit(EXIT_FAILURE);
    }

    /* build out socket components */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = (short) AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(0);

    /* bind */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        write(STDERR_FILENO, "socket error: bind\n", 19);
        exit(EXIT_FAILURE);
    }
    length = sizeof(serv_addr);

    if(getsockname(sockfd, (struct sockaddr *)&serv_addr, (socklen_t *)&length) < 0)
    {
        write(STDERR_FILENO, "socket error: name\n", 19);
        exit(EXIT_FAILURE);
    }
    
    sprintf(buffer, "server started at 127.0.0.1:%d\n", ntohs(serv_addr.sin_port));
    write(STDOUT_FILENO, buffer, BUFFERSIZE);
    bzero(buffer, BUFFERSIZE);

    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    
    for(;;)
    {
        /* Accept connection(s) */
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) {
            write(STDERR_FILENO, "socket error: accept\n", 21);
            exit(EXIT_FAILURE);
        }

        pid = fork();
        if (pid == 0) {
            close(sockfd);

            /* read nickname sent from client */
            if (read(newsockfd,buffer,(size_t)BUFFERSIZE) < 0) {
                write(STDERR_FILENO, "error reading client message\n", 29);
                exit(EXIT_FAILURE);
            }
            /* add client to list */
            *numcli = *numcli + 1;
            strcpy( cli.nick, buffer);
            cli.endpoint = newsockfd;
            printf("current number of clients: %d\n", *numcli);
            /* TODO add client to client list to keep track of connections */
            /*addcli(cli_list, cli, numcli);*/
            printf("nick received: %s\n", cli.nick);

            
            while(read(newsockfd,buffer,(size_t)BUFFERSIZE) > 0)
            {
                /*if () {
                    write(STDERR_FILENO, "error reading client message\n", 29);
                    exit(EXIT_FAILURE);
                }*/
                if(strcmp(buffer, "quit()") == 1) break;
                sprintf(outbuff, "<%s> %s", cli.nick, buffer);
                if(write(newsockfd, outbuff, (size_t)OUTBUFFERSIZE) < 0) {
                    write(STDERR_FILENO, "error writing to client(s)\n", 27);
                    exit(EXIT_FAILURE);
                }
                bzero(buffer,BUFFERSIZE);
                bzero(outbuff,OUTBUFFERSIZE);
            }
            *numcli = *numcli - 1;
            /*if(write(newsockfd, "received", (size_t)BUFFERSIZE) < 0) {
                write(STDERR_FILENO, "error writing to client(s)\n", 27);
                exit(EXIT_FAILURE);
            }*/

            exit(EXIT_SUCCESS);
        } else if (pid < 0) {
            write(STDERR_FILENO, "fork error\n", 11);
            exit(EXIT_FAILURE);
        } else {
            close(newsockfd);
            do {
                wpid = waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
    }

    return 0;
}
/*
    list - the list of all current clients
    sender - the socketfd of the sender
    numcli - current number of connected clients
    buffer - message to be sent to ther clients
*/
int relaymsg(struct Client *list, int sender, int numcli, char *buffer)
{
    int i;
    
    for ( i = 0; i < numcli; i++)
    {
        if(write(list[i].endpoint, buffer, (size_t)BUFFERSIZE) < 0) {
            write(STDERR_FILENO, "error writing to client(s)\n", 27);
            exit(EXIT_FAILURE);
        }
    }
}
void addcli(struct Client *list, struct Client cli, int num)
{   
    list[num++] = cli;
    list = realloc(list, sizeof(cli) * num );
    if (!list) {
        write(STDERR_FILENO, "Allocation error.\n", 18);
        exit(EXIT_FAILURE);
    }

}