#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/**********************************/
/* Author: Dylan Lettinga         */
/* Date: 07/19/2017               */
/**********************************/
int main(int argc, char* argv[])
{
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int total = 0;
    int index;
    int num;
    int readin[4];

    printf("test");
    /* read incoming dealt hand */
    for(index = 0; index < 1; index++){

        if (read(STDIN_FILENO, &readin[index], sizeof(int)) < 0) {
            write(STDERR_FILENO, "ERROR reading from STDIN\n", 26);
            exit(EXIT_FAILURE);
        }
    }

    /*write(STDOUT_FILENO, "player returns:\n", 16);*/

    for(index = 0; index < 1; index++){

        if (write(STDOUT_FILENO, &readin[index], sizeof(int)) < 0) {
            write(STDERR_FILENO, "ERROR reading from STDIN\n", 26);
            exit(EXIT_FAILURE);
        }
    }



    
    return 0;
}
