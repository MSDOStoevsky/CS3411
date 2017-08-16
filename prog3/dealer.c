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
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#define READ  0
#define WRITE 1

/**********************************/
/* Author: Dylan Lettinga         */
/* Date: 07/19/2017               */
/**********************************/

static int deck_size;
static int *highest_total;
static int *winning;
static int *winning_fd;

/* 
    remove from array and return value
*/
int pop(int *arr, int *size, int loc)
{
    int i;
    int val = arr[loc];
    for(i = loc; i < (*size - 1); i++)
    {
        arr[i] = arr[(i+1)];
    }
    arr[*size] = '\0';

    *size = *size-1;
    return val;
}
/*
    send result of game to player
*/
void sendres(int sock, int res)
{
    if (write(sock,&res,(size_t)sizeof(int)) < 0) {
        write(STDERR_FILENO, "could not send result to player\n", 30);
        exit(EXIT_FAILURE);
    }
}
void refresharr(int *arr)
{
    int temp[52] = {1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8,8,8,8,9,9,9,9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13};
    memcpy(arr, temp, (size_t)sizeof(int) * 52);
}
int main(int argc, char* argv[])
{
    pid_t pid;
    int sockfd, newsockfd;
    int status, player, players, total;
    int card, rdm_card, draw;
    int fd [2];
    int deal[4];
    int deck[52] = 
    {1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8,8,8,8,9,9,9,9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13};
    char buffer[256];
    char cardbuff[2];
    bzero(deal,5);
    bzero(buffer,256);

    /* initialize deck size*/
    deck_size = 52;
    
    /* place game score items in shared memory */
    highest_total = mmap(NULL, sizeof *highest_total, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *highest_total = 0;

    /* winning is the connection to the player that has the highest total */
    winning = mmap(NULL, sizeof *winning, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *winning = 0;

    /* winning_fd is the connection to the winners client */
    winning_fd = mmap(NULL, sizeof *winning_fd, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *winning_fd = 0;


    /* reject an execution with no arguments */
    if(argv[1] == NULL)
    {
        write(STDERR_FILENO, "Usage: dealer <n>\n", 18);
        exit(EXIT_FAILURE);
    }
    else
    {
        if( (players = atoi(argv[1])) < 1)
        {
            write(STDERR_FILENO, "n cannot be less than 1\n", 24);
            exit(EXIT_FAILURE);
        }
    }
    
    /* seed random and begin loop */
    srand ( time(NULL) );
    for(player = 1; player <= players; player++)
    {
        /* generate new deck if out of cards */
        if (deck_size <= 0)
        {
            refresharr(deck);
            deck_size = 52;
        }

        /* draw 4 cards to send to player */
        for(card = 0; card < 4; card++)
        {
            rdm_card = rand() % deck_size;
            deal[card] = pop(deck, &deck_size, rdm_card);
        }

        if( pipe(fd) == -1)
        {
            write(STDERR_FILENO, "pipe error\n", 11);
            exit(EXIT_FAILURE);
        }
        pid = fork();
        if (pid == 0) {
            close(STDIN_FILENO);
            dup2(fd[READ], STDIN_FILENO);

            write(fd[WRITE], &deal[0], sizeof(int));
            if (execvp( "./player", NULL) == -1){
                write(STDERR_FILENO, "Execution error\n", 16);
                exit(EXIT_FAILURE);
            }
            while (read(fd[READ], &total, sizeof(int)) > 0)
                write(STDOUT_FILENO, &total, sizeof(int));
            close(fd[WRITE]);

            /*
            write(STDOUT_FILENO, deal, sizeof(deal));
            dup2( fd[WRITE], STDOUT_FILENO );
            
            printf("%s, %s, %s", deal[0], deal[1], deal[2]);
            if (execvp( deal[0], NULL) == -1){
                write(STDERR_FILENO, "Execution error\n", 16);
                exit(EXIT_FAILURE);
            }
            printf("test\n");*/

        } else if (pid < 0) {
            write(STDERR_FILENO, "fork error\n", 11);
            exit(EXIT_FAILURE);
        } else {
            close(fd[READ]);
            wait(NULL);                /* Wait for child */
            exit(EXIT_SUCCESS);
        }
    }

    /*sendres(*winning_fd, 1);*/
    sprintf(buffer, "total read was %d\n", total);
    write(STDOUT_FILENO, buffer, sizeof(buffer));
    return 0;
}