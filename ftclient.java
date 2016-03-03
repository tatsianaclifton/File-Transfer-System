/************************************************/
/* Name: CS372 Program 2 ftclient.java          */
/* Author: Tatsiana Clifton                     */
/* Date: 2/29/2016                              */
/* Description: The file transfer client for a  */
/*              simple file transfer system.    */
/* Usage:                                       */
/*                                              */
/* Sources: 1. docs.oracle.com/javase/tutorial/ */
/* networking/sockets/index.html                */
/* 2. www.rgagnon.com/javadetails/java-0542.html*/
/************************************************/

import java.io.*;
import java.net.*;

public class ftclient{
   //max file size to be downloaded
   public final static int FILE_SIZE = 104857600;

   public static void main (String [] args) throws IOException{
      //check for correct number of arguments
      if (args.length != 4 || args.length != 5){
         System.err.println("Usage: ftclient <server_host> <server_port> -l|-g <filename> <data_port>");
         System.exit(1);
      }
      //get arguments into varuables 
      String host = args[0];
      int port = Integer.parseInt(args[1]); 
      String command = args[2];
      String file = null;
      int dataPort = 0;
      if (command.equals("-g")){
         if (args.length == 4){
            System.err.println("File must be specified for -g command.");
            System.exit(1);
         }
         file = args[3];
         dataPort = Integer.parseInt(args[4]);
      }
      else if (command.equals("-l")){
         file = null;
         dataPort = Integer.parseInt(args[3]);
      }
      
      try{
          //create control socket
          Socket controlSock = new Socket(host, port);
          System.out.format("Connected to %s on port %d\n.", host, port);
          //writer for control socket
          PrintWriter out = new PrintWriter(controlSock.getOutputStream(), true);
          //reader for control socket
          BufferedReader in = new BufferedReader(new InputStreamReader(controlSock.getInputStream()));
 
          //create data server socket
          ServerSocket serverSock = new ServerSocket(dataPort);
          System.out.format("Waiting for connection on port %d\n.", dataPort);
          
          //create string with commands to sent to the server
          String outCommand = command + " " + dataPort + " " + file;
          System.out.println("Sending command to the server" + outCommand);      
          //send command to the server
          out.println(outCommand);

          //accept connection
          Socket dataSock = serverSock.accept(); 
          System.out.format("Accepted connection on port %d\n.", dataPort);
          
          //check for error messages from the server on control connection
          if (in.ready()){
             System.out.println(in.readLine());
             System.exit(0);
          }
          
          int current = 0; 
          //receive file if command is -g      
          if (command.equals("-g")){
             byte [] byteArr = new byte [FILE_SIZE];
             InputStream is = dataSock.getInputStream();
             FileOutputStream fos = new FileOutputStream(file);
             BufferedOutputStream bos = new BufferedOutputStream(fos);
             int bytesRead = is.read(byteArr, 0, byteArr.length);
             current = bytesRead; 

             do{
                bytesRead = is.read(byteArr, current, (byteArr.length-current));
                if (bytesRead >= 0){
                   current += bytesRead;
                }     
             }while(bytesRead > -1);  
          
             bos.write(byteArr, 0, current);
             bos.flush();
             controlSock.close();
             System.out.println("File " + file + " downloaded (" + current + "bytes read"); 
          }
          if (command.equals("-l")){
             String fileList;
             InputStream is = dataSock.getInputStream();
             BufferedReader br = new BufferedReader(new InputStreamReader(is));
             while ((fileList = br.readLine()) != null){
                System.out.println(fileList);
             } 
             controlSock.close();        
          }  
      }
      catch (UnknownHostException e){
         System.err.println("Don't know about host " + host);
         System.exit(1);
      }
      catch (IOException e){
         System.err.println("Couldn't get I?O for the connection to " + host);
         System.exit(1); 
      }
   }
} 
