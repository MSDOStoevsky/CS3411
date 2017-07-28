#define h_addr h_addr_list[0]
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

/**********************************/
/* Author: Dylan Lettinga         */
/* Date: 07/19/2017               */
/**********************************/
void ttlcards(int sock)
{
    char buffer[256];
    int ret[4];
    int card;
    int total = 0;
    bzero(buffer,256);
    /* read dealt cards into ret */
    if (read(sock, ret, sizeof(ret)) < 0) {
        write(STDERR_FILENO, "ERROR reading from socket\n", 26);
        exit(EXIT_FAILURE);
    }

    /* total up the dealt cards */
    for(card = 0; card < 4; card++)
    {
        sprintf(buffer, "recieved: %d\n", ret[card]);
        write(STDOUT_FILENO, buffer, sizeof(buffer));
        total+=ret[card];
    }
    
    sprintf(buffer, "total: %d\n", total);
    write(STDOUT_FILENO, buffer, sizeof(buffer));
    
    /* send the total to dealer */
    if ( write(sock, &total, sizeof(int)) < 0) {
        write(STDERR_FILENO, "ERROR writing to socket\n", 24);
        exit(EXIT_FAILURE);
    }
}
int main(int argc, char* argv[])
{
    int sockfd, portno, ret;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = 5021;
    server = gethostbyname("localhost");

    /* Create a socket point */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
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

    /* process the dealt hand */
    ttlcards(sockfd);

    /* wait for outcome of match */
    write(STDOUT_FILENO, "waiting for other players to make their moves...\n", 49);
    

    /* read outcome */
    if (read(sockfd, &ret, sizeof(ret)) < 0) {
        write(STDERR_FILENO, "ERROR reading from socket\n", 26);
        exit(EXIT_FAILURE);
    }


    if (ret == -1)
    {
        write(STDOUT_FILENO, "you have lost\n", 14);
        return 0;
    }
    else if (ret == 1)
    {
        write(STDOUT_FILENO, "you have won\n", 13);
        return 0;
    }
    /* while there is a tie, read incoming dealt hands */
    else{
        while(ret == 0)
        {
            write(STDOUT_FILENO, "you have tied with another player, receiving new hand...\n", 56);
            /* process the dealt hand */
            ttlcards(sockfd);
            /* read outcome */
            if (read(sockfd, &ret, sizeof(ret)) < 0) {
                write(STDERR_FILENO, "ERROR reading from socket\n", 26);
                exit(EXIT_FAILURE);
            }
        }
    }

    
    return 0;
}
