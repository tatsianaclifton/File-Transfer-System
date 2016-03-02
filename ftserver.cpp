/************************************************
 * Name: CS372 Program 2 ftserver.cpp           *
 * Author: Tatsiana Clifton                     *
 * Date: 2/29/2016                              *
 * Descriptino: The file transfer server for a  *
 *              simple file transfer system     *
 * Usage:                                       *
 *                                              *
 * Sources: beej.us/guide/bgnet/output/html/    *
 * multipage/index.html                         *
 ************************************************/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

using namespace std;

#define BACKLOG 5 //how many pending connections queue can hold

void setConnect(char *port);

int main(int argc, char *argv[]){
   //check that port is provided
   if (argc != 2){
      cout << stderr << "usage: ftserver <port_number>" << endl;
      exit(1);
   }
   //set connection
   setConnect(argv[1]);
}

/*************************************************************
 Function: sigchld_handler
 Receive: signal
 Return: none
 Description: reap dead child processes
 Source: http://beej.us/quide/bqnet/output/html/multipage/index.html
**************************************************************/
void sigchld_handler(int s){
   //waitpid() might overwrite errno, so we save and restore it
   int saved_errno = errno;

   while (waitpid(-1, NULL, WNOHANG) > 0);

   errno = saved_errno;
} 

/**************************************************************
 Function: get_in_addr
 Receive: pointer to sockaddr struct
 Return: IPv4 or IPv6
 Description: gets IPv4 or IPv6 in sockaddr struct
 Sources: http://beej.us/quide/bqnet/output/html/multipage/index.html
****************************************************************/
void *get_in_addr(struct sockaddr *sa){
   if(sa->sa_family == AF_INET){
      return &(((struct sockaddr_in*)sa)->sin_addr);
   }
   return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/****************************************************************
 Function: setConnect
 Receive: port number
 Return: none
 Description: set up the server to listen on specified port
 number and except request and set up connection for transfer file
 Sources: http://beej.us/quide/bqnet/output/html/multipage/clientserver.html
*****************************************************************/
void setConnect(char *port){
   int sockfd, new_fd;
   struct addrinfo hints, *servinfo, *p;
   struct sockaddr_storage their_addr;
   socklen_t sin_size;
   struct sigaction sa;
   int yes = 1;
   char s[INET6_ADDRSTRLEN];
   int rv;

   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;

   if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0){
      cout << stderr << "getaddrinfo: " << gai_strerror(rv) << endl;
      exit(1);
   }
   
   for (p = servinfo; p != NULL; p = p->ai_next){
      if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1){
         perror("server: socket");
         continue;
      }
      
      if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
             sizeof(int)) == -1){
         perror("setsockopt"); 
         exit(1);
      }

      if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
         close(sockfd);
         perror("server: bind");
         continue;
      }
      break;
   }

   freeaddrinfo(servinfo);

   if (p == NULL){
      cout << stderr << "server: failed to bind" << endl;
      exit(1);
   }

   if (listen(sockfd, BACKLOG) == -1){
      perror("listen");
      exit(1);
   }

   sa.sa_handler = sigchld_handler;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = SA_RESTART;
   if (sigaction(SIGCHLD, &sa, NULL) == -1){
      perror("sigaction");
      exit(1);
   }

   cout << "server; waiting for connections" << endl;

   while (1){
      sin_size = sizeof their_addr;

      new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
      if (new_fd == -1){
         perror("accept");
         continue;
      }

      inet_ntop(their_addr.ss_family,
          get_in_addr((struct sockaddr *)&their_addr),
          s, sizeof s);
      cout << "server: got connection from" << s << endl;
   }
   //close connection
   close(new_fd);
}
