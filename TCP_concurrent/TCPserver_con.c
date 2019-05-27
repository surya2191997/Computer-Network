/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <openssl/md5.h> 


void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{

     int sockfd, newsockfd, portno, clilen;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
    

     // Create a socket at given port and listen for connections
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     listen(sockfd,5);



     // Accept the connection at a new socket (TCP)

     while(1)
     {
     // parent process waiting for new connection
     clilen = sizeof(cli_addr);
     newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
     if (newsockfd < 0) 
          error("ERROR on accept");
     

      // child process is created for serving each new clients 
      pid_t pid = fork();
      if(pid == 0) 
      {

        close(sockfd);


     // 1. Read the filename and file size in the buffer
     bzero(buffer,256);
     n = read(newsockfd,buffer,255);
     if (n < 0) error("ERROR reading from socket");
     printf("Client Says : %s\n",buffer);
     n = write(newsockfd,"Acknowledged ! Send the file",30);
     if (n < 0) error("ERROR writing to socket");
     
    
     // 2. Open the file and store the file size
     int k, j, i, len1, len2, fileSize, total=0, bytesReceived = 1;
     for(i = 0; i < 256; i++ )
     	if(buffer[i]==' ')
     		break;

     len1 = i + 1;
     char size[len1];
     size[i] = '\0';
     for(j = 0 ; j < i ;j++)
     	size[j] = buffer[j];
     sscanf(size, "%d", &fileSize);
    

  	for(j=i+1; j<256; j++)
  		if(buffer[j]==' ')
  			break;

  	len2 = j - i;
  	char filename[len2];
    filename[len2-1] = '\0';
  	for(k = i+1; k<j;k++)
  	  filename[k-i-1] = buffer[k];


  
     FILE *fp;	
     fp = fopen(filename, "a");
     if(NULL == fp)
     {
     	printf("Error opening file");
     	return 1;
     }
     

     //3. Read data from the socket (in chunks of 256 bytes)
    unsigned char c[MD5_DIGEST_LENGTH];
    MD5_CTX mdContext;
    MD5_Init (&mdContext);
     while(bytesReceived)
     {
     	bytesReceived = read(newsockfd,buffer,256);
     	MD5_Update (&mdContext, buffer, bytesReceived);
     	total += bytesReceived; 
     	printf("Bytes Received : %d, Total : %d\n", bytesReceived, total);
     	fwrite(buffer, 1, bytesReceived, fp);
     	if(total == fileSize)
     	break; 
     }

    MD5_Final (c,&mdContext);

    printf("\nFILE RECEIVED !\n");
   	printf("Sending MD5 checksum to client...\n");
   	sleep(2);
   	write(newsockfd, c, MD5_DIGEST_LENGTH);
   	printf("\nSENT !\n");

    if(bytesReceived < 0)
     	printf("\n Read Error \n");

     exit(0);
     	
     }


     
        close(newsockfd); // sock is closed BY PARENT
      

 }



     
     return 0; 
}
