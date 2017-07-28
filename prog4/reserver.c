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
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFERSIZE 256
#define READ  0
#define WRITE 1

/**********************************/
/* Author: Dylan Lettinga         */
/* Date: 08/04/2017               */
/**********************************/
/* turn string into array of strings based on delimiter */
char **split(char *string, const char *delim);
/* fork and execute command */
static int exec(char *in_cmd, int out);
int main()
{
    pid_t pid, wpid;
    socklen_t clilen;
    int sockfd, newsockfd;
    int status, length;
    struct sockaddr_in serv_addr, cli_addr;
    char buffer[256];
    bzero(buffer,256);

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

            /* read commands sent from client */
            if (read(newsockfd,buffer,(size_t)256) < 0) {
                write(STDERR_FILENO, "error reading client message\n", 29);
                exit(EXIT_FAILURE);
            }

            if (exec(buffer, newsockfd) < 0)
            {
                write(STDERR_FILENO, "bad command sent\n", 17);
            }
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
char **split(char *string, const char *delim)
{
    int position = 0, bufsize = BUFFERSIZE;
    char **argv = malloc(bufsize);
    char *arg;

    if(!argv) 
    {
        write(STDERR_FILENO, "Allocation error.\n", 18);
        exit(EXIT_FAILURE);
    }

    arg = strtok(string, delim);
    while (arg != NULL) {
        argv[position++] = arg;

        /* increase array size */
        if (position >= bufsize) {
            bufsize += BUFFERSIZE;
            argv = realloc(argv, bufsize);
            if (!argv) {
                write(STDERR_FILENO, "Allocation error.\n", 18);
                exit(EXIT_FAILURE);
            }
        }
        arg = strtok(NULL, delim);
    }
    argv[position] = NULL;
    return argv;
}
static int exec(char *in_cmd, int out)
{
    pid_t pid, wpid;
    int status;
    const char delim[5] = " \t\r\n\a";
    char **cmd = split(in_cmd, delim);

    /* ignore if no input */
    if (cmd[0] == NULL) return -1;

    pid = fork();
    if (pid == 0) {
        /* copy out to stdout and stderr*/
        dup2(out, STDOUT_FILENO);
        dup2(out, STDERR_FILENO);
        close(out);

        if (execvp( cmd[0], cmd) == -1)
        {
            free(cmd);
            exit(EXIT_FAILURE);
        }

    } else if (pid < 0) {
        write(STDERR_FILENO, "fork error\n", 11);
        return -1;
    } else {
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    
    free(cmd);
    return 0;
}