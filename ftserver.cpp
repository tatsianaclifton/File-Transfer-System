/************************************************
 * Name: CS372 Program 2 ftserver.cpp           *
 * Author: Tatsiana Clifton                     *
 * Date: 2/29/2016                              *
 * Descriptino: The file transfer server for a  *
 *              simple file transfer system     *
 * Usage: ftserver <port>                       *
 *                                              *
 * Sources: 1. beej.us/guide/bgnet/output/html/ *
 * multipage/index.html                         *
 * 2. codereview.stackexchange.com/questions/   *
 * 13461/two-way-communication-in-tcp-server-   *
 * client-implementation                        *
 * 3. www.cplusplus.com/forum/general/17892     *
 * 4. man7.org/linux/man-pages/man2/getpeername *
 *   .2.html                                    *
 * 5. stackoverflow.com/questions/612097/how-   *
 * can-i-get-the-list-of-files-in-a-directory-  *
 * using-c-or-c                                 *
 * 6. www.cplusplus.com/reference/cstdio/fread/ *
 ************************************************/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sstream> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <vector>
#include <algorithm>
#include <iterator>
#include <dirent.h>

using namespace std;

#define BACKLOG 5 //how many pending connections queue can hold
#define MAXDATASIZE 500 //length of the command from the client

void startup(char *port);
void parseRequest(int controlSock);
void handleRequest(int controlSock, char *command, char *host, char *dataPort, char *file);

int main(int argc, char *argv[]){
   //check that port is provided
   if (argc != 2){
      cout << stderr << "usage: ftserver <port_number>" << endl;
      exit(1);
   }
   //set connection
   startup(argv[1]);
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
 Function: startup
 Receive: port number
 Return: none
 Description: set up the server to listen on specified port
 number and except request and set up connection for transfer file
 Sources: http://beej.us/quide/bqnet/output/html/multipage/clientserver.html
*****************************************************************/
void startup(char *port){
   int sockfd, controlSock;
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

   cout << "server: waiting for connections" << endl;

   while (1){
      sin_size = sizeof their_addr;

      controlSock = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
      if (controlSock == -1){
         perror("accept");
         continue;
      }

      inet_ntop(their_addr.ss_family,
          get_in_addr((struct sockaddr *)&their_addr),
          s, sizeof s);
      cout << "server: got connection from " << s << endl;
      parseRequest(controlSock);
   }
   //close connection
   close(controlSock);
}

/************************************************************
 * Function: handleRequest           
 * Receive: socket descriptor controlSock
 * Return: none
 * Description: receives command from the cleint
 * Sources: codereview.stackexchange.com/questions/13461/two-way
 * -communication-in-tcp-server-client-implementation
 *************************************************************/
void parseRequest (int controlSock){
   char buffer[MAXDATASIZE];
   int numbytes;
   //clear buffer
   memset(buffer, 0, MAXDATASIZE);
   //count how many bytes received, if none show an error 
   if ((numbytes = recv(controlSock, buffer, sizeof buffer, 0)) == -1){
      perror("recv");
      exit(1);
   }
   //get rid of new line character at the end of buffer
   int ln = strlen(buffer) - 1;
   if (buffer[ln] == '\n'){
      buffer[ln] = '\0';
   }
   cout << buffer << endl;
   //parse received command  
   char  *command; //for holding -g or -l
   char *dataPort; //for holding port to connect for sending file or dir 
   char *file; //for holding name of the file
   
   command = strtok(buffer, " \n");
   dataPort = strtok(NULL, " \n");
   
   if (strcmp(command, "-g") == 0){
      file = strtok(NULL, " \n"); 
   }
   cout << "command " << command << endl;
   cout << "port " << dataPort << endl;
   cout << "file " << file << endl;
   
   //get address of the client;
   //sources: man7.org/linux/man-pages/man2/getpeername.2.html
   //beej.us/guide/bgnet/output/html/multipage/getpeernameman.html
   socklen_t len;
   struct sockaddr_storage addr;  
   char host[INET6_ADDRSTRLEN];
   len = sizeof addr;
   getpeername(controlSock, (struct sockaddr*)&addr, &len);  
   //deal with both IPv4 and IPv6
   if (addr.ss_family == AF_INET){
      struct sockaddr_in *s = (struct sockaddr_in *)&addr;
      inet_ntop(AF_INET, &s->sin_addr, host, sizeof host); 
   }
   else{
      struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
      inet_ntop(AF_INET6, &s->sin6_addr, host, sizeof host); 
   }
   //call function to set data connection and handle request
   handleRequest(controlSock, command, host, dataPort, file);
} 

void handleRequest(int controlSock, char *command, char *host, char *dataPort, char *file){
   int dataSock;
   struct addrinfo hints, *servinfo, *p;
   int rv;
   char s[INET6_ADDRSTRLEN];
   cout << "connect to " << host << endl;
   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_UNSPEC;//set that both IPv4 and v6 can be looked
   hints.ai_socktype = SOCK_STREAM;//set socket to be SOCK_STREAM
   //get info about host, otherwise show error 
   if ((rv = getaddrinfo(host, dataPort, &hints, &servinfo)) != 0){
      cout << stderr << "getaddrinfo:" << gai_strerror(rv) << endl;
      return;
   }
   //loop through all the results and connect to the first we can
   for (p = servinfo; p != NULL; p = p->ai_next){
      //get new socket descriptor, otherwise show error
      if ((dataSock = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1){
         perror("client: socket");
         continue;
      }
      //connect to the socket, otherwise show error
      if (connect(dataSock, p->ai_addr, p->ai_addrlen) == -1){
         close(dataSock);
         perror("client: connect");
         continue;
      }
      break;
   }
   //if all results were tried and connection was not established
   if (p == NULL){
      cout << stderr << "client: failed to connect" << endl;
      exit(1); 
   }
   printf("connected to the client", host);
   freeaddrinfo(servinfo);  
   //if command is -l get list of files from current directory
   //source: stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
   char buffer[MAXDATASIZE];
   //clear buffer
   memset(buffer, 0, MAXDATASIZE);
   if (strcmp(command, "-l") == 0){
      DIR *dir;
      struct dirent *ent;
      //if there is files in current directory
      if ((dir = opendir(".")) != NULL){
         //send list to the client
         while ((ent = readdir(dir)) != NULL){
            sprintf(buffer, "%s\n", ent->d_name);
            send(dataSock, buffer, strlen(buffer) + 1, 0);
         }
         closedir(dir); //close opened directory
      }
      close(dataSock); //close data socket
   }
   //send file
   //source: www.cplusplus.com/reference/cstdio/fread/
   if (strcmp(command, "-g") == 0){
      //get rid of new line character at the end of file name
      int ln = strlen(file) - 1;
      if (file[ln] == '\n'){
         file[ln] = '\0';
      }
      char *buff;
      long fileSize;
      size_t result;
      FILE *fd;
      fd = fopen(file, "r");
      //if cannot open the file, send error message to the client
      if (!fd){
         sprintf(buffer, "No such file on the directory.\n");
         send(controlSock, buffer, strlen(buffer) + 1, 0);
         close(dataSock); 
      }
      //get file size
      fseek (fd, 0, SEEK_END);
      fileSize = ftell(fd);
      rewind(fd);
      //allocate memory to contain the whole file:
      buff = (char*) malloc (sizeof(char)*fileSize);
      if (buff == NULL){
         cout << stderr << "Memory error" << endl;
         exit(1);   
      }
      //copy file into buffer
      result = fread(buff, 1, fileSize, fd);
      if (result != fileSize){
         char error[20] = "reading error"; 
         cout << stderr << "Reading error" << endl;
         send(controlSock, error, strlen(error) + 1, 0);
         exit(1);   
      } 
      send(dataSock, buff, strlen(buff), 0);
      fclose(fd);
      free(buff);
      close(dataSock);
   }   
}

