/**************************************************
File: README.txt
Author: Tatsiana Clifton
Date: 3/6/2016
Description: Instructions how to compile and run ftserver.cpp
             and ftclient.java programs
Note: the client can accept the file with size up to 200MB			 
***************************************************/

*****************
     COMPILE
*****************
1. Place ftserver.cpp in one directory and
with command line:

g++ ftserver.cpp -o ftserver

2. Place ftclient.java in other directory and
with command line:

javac ftclient.java

****************
     USAGE
****************
To run programs on one computer two windows are required, one for the server
and one for the client

1. ftserver starts with the command line 

./ftserver <port_number> 

and waits on <port_number> for a client

2. ftclient connects to the server with the command line 

java ftclient <hostname> <port_number> <command> <data_port> <file>

<command> can be -g or -l; if command is -g <file> is required;
if command is -l <file> must be omitted

********************************** 
   CLOSE CONNECTION AND TERMINATE
**********************************   
1. ftserver closes the data connection and terminates with CTRL+C 
2. ftclient closes the control connection after the file is transferred or 
list of files is received

*******************
     TESTING
*******************
The programs are tested to run on the same flip server.

*******************
   EXTRA CREDIT
*******************
If a user requested to transfer a file that already exists,
the client program asks permition to override with the prompt:
"Do you want to override Y/N?"

*******************
   RESOURCES      
*******************
1. Beej's Guide to Network Programming
Using Internet Sockets
Brian "Beej Jorgensen" Hall
beej.us/guide/bgnet/output/html/multipage/index.html 
2. codereview.stackexchange.com/questions/13461/two-way-communication-in-tcp-server-client-implementation
3. www.cplusplus.com/forum/general/17892           
4. man7.org/linux/man-pages/man2/getpeername.2.html
5. stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
6. www.cplusplus.com/reference/cstdio/fread/  
7. docs.oracle.com/javase/tutorial/networking/sockets/index.html           
8. www.rgagnon.com/javadetails/java-0542.html       
9. stackoverflow.com/questions/11968265/return-more-than-one-variable-from-java-method
10. stackoverflow.com/questions/5694385/getting-the-filenames-of-all-files-in-a-folder
