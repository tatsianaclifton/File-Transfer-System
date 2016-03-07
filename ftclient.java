/******************************************************/
/* Name: CS372 Program 2 ftclient.java                */
/* Author: Tatsiana Clifton                           */
/* Date: 2/29/2016                                    */
/* Description: The file transfer client for a        */
/*              simple file transfer system.          */
/* Usage: ftclient <server_host> <server_port>        */
/* -l|-g <data_port> <file_name>                      */
/*                                                    */
/* Sources: 1. docs.oracle.com/javase/tutorial/       */
/* networking/sockets/index.html                      */
/* 2. www.rgagnon.com/javadetails/java-0542.html      */    
/* 3. For returning two variables from function:      */
/*   stackoverflow.com/questions/11968265/return-more-*/
/*  than-one-variable-from-java-method                */
/* 4. For getting the filenames of all files:         */
/*   stackoverflow.com/questions/5694385/getting-the- */
/*  filenames-of-all-files-in-a-folder                */
/*                                                    */          
/* Note: The client accepts file size up to 200MB     */
/* EXTRA CREDIT: if the file already exists, the      */
/* program asks if the user wants to override existing*/
/* file with the prompt:"Do you want to override Y/N?"*/
/******************************************************/

import java.io.*;
import java.net.*;
import java.util.*;

public class ftclient{

   //max file size to be downloaded 200MB
   public final static int FILE_SIZE = 209715200;
   //global variables
   public static String host; //will hold host name
   public static int port; //will hold port number for control connection 
   public static String command; //will hold command -g or -l
   public static int dataPort; //will hold port number for data connection
   public static String file = ""; //holds file name

   /*Function: main
     Description: the main method that verifies arguments 
     and calls other functions*/
   public static void main(String[] args) throws IOException{
      //check for correct number of arguments
      if (args.length != 4 && args.length != 5){
         System.err.println("Usage: ftclient <server_host> <server_port> -l|-g <data_port> <file_name>");
         System.exit(1);
      }
      try{
         //connect to the server and accept connection from the server
         initiateContact(args);
         //map will hold control and data sockets 
         Map<String, Socket> result = new HashMap<String, Socket>(); 
         //make request to the server and save sockets into variable 
         result = makeRequest();
         Socket control, data; 
         control = result.get("control");
         data = result.get("data");
         //receive file or list of files
         receiveFile(control, data);
      }
      catch(IOException e){
         System.err.println(e.getMessage());
      }
   }

      
   /*Function: initiateContact
     Description: saves arguments into appropriate variables,
     verifies commands, checks that the file does not exist 
     in the directory */
   public static void initiateContact(String[] args){
      //get arguments into variables 
      host = args[0];
      port = Integer.parseInt(args[1]); 
      command = args[2];
      dataPort = Integer.parseInt(args[3]);
      //verify that command is -g or -l
      if (!command.equals("-g") && !command.equals("-l")){
         System.err.println("Command must be -g or -l");
         System.exit(1);
      }
      //if command is -g, file must be specified, get file name 
      //into variable, if not show error
      if (command.equals("-g")){
         if (args.length == 4){
            System.err.println("File must be specified for -g command.");
            System.exit(1);
         }
         file = args[4];
      }
      //get list of all files
      File folder = new File(".");
      File[] list = folder.listFiles();
      //verify that file that was requested not in the directory already  
      for (int i = 0; i < list.length; i++){
         if(list[i].isFile()){
            if (file.equals(list[i].getName())){
               System.err.println("File already exists.");
               System.out.println("Do you want to override Y/N?");
               Scanner scan = new Scanner(System.in);
               String input = scan.next();
               if(input.equals("N")){
                  System.exit(0);
               }
            }
         }
      }
   }
   
   /*Function: makeRequest
     Description: connects to the control socket, sends command to this
     socket; creates data socket and accepts connection on this socket;
     returns control and data sockets*/ 
   public static Map<String, Socket> makeRequest() throws IOException{  
      Map<String, Socket> result = new HashMap<String, Socket>();
      try{
          //create control socket
          Socket controlSock = new Socket(host, port);
          //writer for control socket
          PrintWriter out = new PrintWriter(controlSock.getOutputStream(), true);
          //reader for control socket
          BufferedReader in = new BufferedReader(new InputStreamReader(controlSock.getInputStream()));
          //create data server socket
          ServerSocket serverSock = new ServerSocket(dataPort);
          System.out.format("client: Waiting for connection on port %d\n", dataPort);
          System.out.format("client: Connected to %s on port %d\n", host, port);
          //create string with commands to sent to the server
          String outCommand = command + " " + dataPort + " " + file;
          System.out.println("client: Sending command to the server " + outCommand);      
          //send command to the server
          out.println(outCommand);
          //accept connection
          Socket dataSock = serverSock.accept(); 
          System.out.format("client: Accepted connection on port %d\n", dataPort);
          InputStream is = dataSock.getInputStream();
          BufferedReader br = new BufferedReader(new InputStreamReader(is));
          //check for messages from the server on control connection
          String response = "";
          //while server not sending data to dataSocket
          while (!br.ready()){
             //if server has a message on control connection
             if(in.ready()){ 
                //read this message
                response = in.readLine();  
                //display message     
                System.out.println(response);
                controlSock.close();
                System.exit(1);
             }
             continue;
          }
          
         result.put("control", controlSock);
         result.put("data", dataSock);
      }
      catch (UnknownHostException e){
         System.err.println("Don't know about host " + host);
         System.exit(1);
      }
      catch (IOException e){
         System.err.println("Couldn't get I/O for the connection to " + host);
         System.exit(1); 
      }
      return result;
   }

   /*Function: receiveFile
     Description: depending on the command receives the file
     or receives the list of the files from the server. If it is 
     a list the function displays the list*/
   public static void receiveFile(Socket controlSock, Socket dataSock) throws IOException{
      try{
         //input stream for data socket
         InputStream is = dataSock.getInputStream();
         //buffered reader for data socket
         BufferedReader br = new BufferedReader(new InputStreamReader(is));
         int current = 0; 
         //receive file if command is -g      
         if (command.equals("-g")){
            //will hold file read from input stream
            byte [] byteArr = new byte [FILE_SIZE];
            //output stream for writing data into file
            FileOutputStream fos = new FileOutputStream(file);
            //for writing bytes into output stream
            BufferedOutputStream bos = new BufferedOutputStream(fos);
            //read
            int bytesRead = is.read(byteArr, 0, byteArr.length);
            current = bytesRead; 
            do{
               bytesRead = is.read(byteArr, current, (byteArr.length-current));
               if (bytesRead >= 0){
                  current += bytesRead;
               }     
            }while(bytesRead > -1);  
            //write
            bos.write(byteArr, 0, current);
            //clear buffered output stream 
            bos.flush();
            //close control socket
            controlSock.close();
            System.out.println("Transfer completed. File " + file + " downloaded (" + current + " bytes read)"); 
         }
         //if command is -l, read the list of files and display it 
         if (command.equals("-l")){
            String fileList;
            while ((fileList = br.readLine()) != null){
               System.out.println(fileList);
            }
            //close control socket when done    
            controlSock.close();        
         }  
      }     
      catch (IOException e){
         System.err.println(e.getMessage());
      }
   } 
} 
