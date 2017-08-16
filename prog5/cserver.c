#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#define OBUFFERSIZE 182
#define BUFFERSIZE 128
#define NICKSIZE 52

struct Client {
    char nick[NICKSIZE];
    int endpoint;
};
int main(int argc , char *argv[])
{
    int opt = 1;
    int serversock, addrlen, new_socket, max_clients = 30, activity, valread, sd;
    int i, j;
    int max_sd;
    struct sockaddr_in address;
    fd_set readfds;

    struct Client client_socket[30];

    char clinick[NICKSIZE];
    char obuffer[OBUFFERSIZE];
    char buffer[BUFFERSIZE];
    bzero(clinick,NICKSIZE);
    bzero(obuffer,OBUFFERSIZE);
    bzero(buffer,BUFFERSIZE);
    
    /* initialize client socket arr */
    for (i = 0; i < max_clients; i++) 
    {
        bzero(client_socket[i].nick, NICKSIZE);
        client_socket[i].endpoint = 0;
    }
      
    if( (serversock = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
    {
        write(STDERR_FILENO, "socket failure\n", 15);
        exit(EXIT_FAILURE);
    }
  
    if( setsockopt(serversock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        write(STDERR_FILENO, "sockopt error\n", 14);
        exit(EXIT_FAILURE);
    }
  
    /* build out socket components */
    bzero((char *) &address, sizeof(address));
    address.sin_family = (short) AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(0);
      
    if (bind(serversock, (struct sockaddr *)&address, sizeof(address))<0) 
    {
        write(STDERR_FILENO, "bind failure\n", 13);
        exit(EXIT_FAILURE);
    }
    addrlen = sizeof(address);

    if(getsockname(serversock, (struct sockaddr *)&address, (socklen_t *)&addrlen) < 0)
    {
        write(STDERR_FILENO, "socket error: name\n", 19);
        exit(EXIT_FAILURE);
    }

    sprintf(buffer, "server started at 127.0.0.1:%d\n", ntohs(address.sin_port));
    write(STDOUT_FILENO, buffer, BUFFERSIZE);
    bzero(buffer, BUFFERSIZE);
     
    if (listen(serversock, 5) < 0)
    {
        write(STDERR_FILENO, "socket error: listen\n", 21);
        exit(EXIT_FAILURE);
    }
    
    /* begin listening for activity */
    while(1) 
    {
        FD_ZERO(&readfds);
        FD_SET(serversock, &readfds);
        max_sd = serversock;
        
        for ( i = 0 ; i < max_clients ; i++) 
        {
            sd = client_socket[i].endpoint;
             
            if(sd > 0)
                FD_SET( sd , &readfds);
             
            if(sd > max_sd)
                max_sd = sd;
        }
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
    
        if ((activity < 0) && (errno!=EINTR)) 
        {
            write(STDERR_FILENO, "select error\n", 13);
        }

        /* i/o detected on server */
        if (FD_ISSET(serversock, &readfds)) 
        {
            if ((new_socket = accept(serversock, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                write(STDERR_FILENO, "accept error\n", 13);
                exit(EXIT_FAILURE);
            }

            /* read nickname sent from client */
            if (read(new_socket,buffer,NICKSIZE) < 0) {
                write(STDERR_FILENO, "error reading client nickname\n", 30);
                exit(EXIT_FAILURE);
            }
              
            for (i = 0; i < max_clients; i++) 
            {
                if( client_socket[i].endpoint == 0 )
                {
                    strcpy(client_socket[i].nick, buffer);
                    client_socket[i].endpoint = new_socket;
                    printf("%s has joined the chat\n", buffer);
                    break;
                }
            }
            bzero(buffer, BUFFERSIZE);
        }
          
        for (i = 0; i < max_clients; i++) 
        {
            sd = client_socket[i].endpoint;
            strcpy(clinick, client_socket[i].nick);
              
            if (FD_ISSET( sd , &readfds)) 
            {
                if ((valread = read( sd , buffer, BUFFERSIZE)) == 0)
                {
                    getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
                    sprintf(buffer, "%s disconnect at address %d:%d\n", client_socket[i].nick, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    write(STDOUT_FILENO, buffer, BUFFERSIZE);
                    bzero(buffer, BUFFERSIZE);

                    close( sd );
                    bzero(client_socket[i].nick, NICKSIZE);
                    client_socket[i].endpoint = 0;
                }
                  
                else
                {
                    /* relay messages to all other connected clients */
                    for (j = 0; j < max_clients; j++) 
                    {
                        sd = client_socket[j].endpoint;
                        if(sd != 0){

                            sprintf(obuffer, "<%s> %s", clinick, buffer);
                            obuffer[OBUFFERSIZE] = '\0';
                            write(sd, obuffer, OBUFFERSIZE);
                        }
                    }
                }
            }
            bzero(obuffer, OBUFFERSIZE);
            bzero(buffer, BUFFERSIZE);
            bzero(clinick, NICKSIZE);
        }
    }
      
    return 0;
} 