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
#include <time.h>
#define PORTNO 5021

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
    send values to player and return response
*/
int sendtop (int sock, int *cards, int player) {
    int total, index;
    char buffer[256];
    bzero(buffer,256);
    for(index = 0; index < 4; index++)
    {
        if (write(sock,&(cards[index]),(size_t)sizeof(int)) < 0) {
            write(STDERR_FILENO, "could not deal cards to player\n", 30);
            exit(EXIT_FAILURE);
        }
    }
    if (read(sock,&total,(size_t)sizeof(total)) < 0) {
        write(STDERR_FILENO, "could not read players total\n", 29);
        exit(EXIT_FAILURE);
    }
    
    /* ignore outrageous totals */
    if(total < 4 || total > 52)
    {
        write(STDOUT_FILENO, "received bad total from player\n", 31);
        return 0;
    }
    sprintf(buffer, "player %d's total was: %d\n", player, total);
    write(STDOUT_FILENO, buffer, sizeof(buffer));
    return total;
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
    pid_t pid, wpid;
    socklen_t clilen;
    int sockfd, newsockfd;
    int status, player, players, total;
    int card, rdm_card, draw;
    struct sockaddr_in serv_addr, cli_addr;
    int deal[4];
    int deck[52] = 
    {1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8,8,8,8,9,9,9,9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13};
    char buffer[256];
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
    serv_addr.sin_port = htons(PORTNO);

    /* bind */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        write(STDERR_FILENO, "socket error: bind\n", 19);
        exit(EXIT_FAILURE);
    }

    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    
    /* seed random and begin loop */
    srand ( time(NULL) );
    player = 0;
    for(player = 1; player <= players; player++)
    {
        /* generate new deck if out of cards */
        if (deck_size <= 0)
        {
            refresharr(deck);
            deck_size = 52;
        }

        /* Accept connection(s) */
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) {
            write(STDERR_FILENO, "socket error: accept\n", 21);
            exit(EXIT_FAILURE);
        }
        
        /* draw 4 cards to send to player */
        for(card = 0; card < 4; card++)
        {
            rdm_card = rand() % deck_size;
            draw = pop(deck, &deck_size, rdm_card);
            deal[card] = draw;
            sprintf(buffer, "dealing card %d to player %d\n", draw, player);
            write(STDOUT_FILENO, buffer, sizeof(buffer));
        }

        pid = fork();
        if (pid == 0) {
            close(sockfd);

            /* send hand to player and retreive the total */
            total = sendtop(newsockfd, deal, player);

            /* check if current player is winning */
            if(total < *highest_total)
            {
                sendres(newsockfd, -1);
                sprintf(buffer, "player %d is knocked out, with a score of %d\n", player, total);
                write(STDOUT_FILENO, buffer, sizeof(buffer));
            }
            else if(total > *highest_total)
            {
                if(*winning != 0){
                    sendres(*winning_fd, -1);
                    sprintf(buffer, "player %d is knocked out, with a score of %d\n", *winning, *highest_total);
                    write(STDOUT_FILENO, buffer, sizeof(buffer));
                }
                /* set curr winner to curr player connection */
                *winning = player;
                *winning_fd = newsockfd;
                *highest_total = total;
            }
            else
            {  
                sendres(newsockfd, 0);
                sendres(*winning_fd, 0);
                sprintf(buffer, "player %d has tied with player %d\n", player, *winning);
                write(STDOUT_FILENO, buffer, sizeof(buffer));
            }

            exit(EXIT_SUCCESS);
        } else if (pid < 0) {
            write(STDERR_FILENO, "fork error\n", 11);
            exit(EXIT_FAILURE);
        } else {
            /*close(newsockfd);*/
            do {
                wpid = waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
    }

    sendres(*winning_fd, 1);
    sprintf(buffer, "the winner is: player %d, with a score of %d!\n", *winning, *highest_total);
    write(STDOUT_FILENO, buffer, sizeof(buffer));
    return 0;
}