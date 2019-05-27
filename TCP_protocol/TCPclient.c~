#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <openssl/md5.h> 
#include <stdlib.h>


void error(char *msg)
{
    perror(msg);
    exit(0);
}



int main(int argc, char *argv[])
{

    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];
    

    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    

    // Get server credentials from command line and connect
    portno = atoi(argv[2]);
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
     
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n",argv[1]);
        exit(0);
    }


    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
     


    // Connected to server 
    


    // Ask user for the file name and compute file size
    char filename[100];
    printf("Enter the name of the file you want to send :\n");
    scanf("%s", filename);
    FILE *fp = fopen(filename, "rb");
    if(fp == NULL)
    {
    	printf("File open error");
    	return 1;
    }
    fseek(fp, 0L, SEEK_END);
	int sz = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	printf("The size of file is : %d \n", sz);
	sleep(1);

    
    // 1. Client sends hello message along with file name and file size
	printf("Sending hello message to server...\n");
	sleep(2);
    bzero(buffer,256);
    //strcpy(buffer, "Hello, File Name : sample_file.txt, Size : 16Kb");
    sprintf(buffer, "%d %s Hello", sz, filename);
  	n = write(sockfd,buffer,strlen(buffer));
    if (n < 0) 
         error("ERROR writing to socket");
    



     // 2. Client reads acknowledgement response of the server
    printf("Server Says : ");
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0) 
         error("ERROR reading from socket");
    printf("%s\n",buffer);
    
    


    // 3. Start the intended file transfer
    printf("Initiating file transfer...\n");
    sleep(2);

    unsigned char c[MD5_DIGEST_LENGTH];
    MD5_CTX mdContext;
    MD5_Init (&mdContext);
    while(1)
    {

    	bzero(buffer, 256);
    	int nread = fread(buffer,1,256,fp);
    	MD5_Update (&mdContext, buffer, nread);
    	printf("Bytes read %d \n", nread);
    	
    // if read was success, send data
 	if(nread > 0)
 	{
 		printf("Sending... \n");
 		write(sockfd, buffer, nread);
 	}	   


	// nread < 256 indicates EOF 	
 	if(nread < 256)
 	{
 		if(feof(fp))
 			printf("End of file\n");
 		if(ferror(fp))
 			printf("Error reading\n");
 		break;
 	}
 	sleep(0.5);
 	
    }

    MD5_Final (c,&mdContext);
    unsigned char rec[MD5_DIGEST_LENGTH];
    bzero(buffer, MD5_DIGEST_LENGTH);


     printf("\nFILE TRANSFER SUCCESSFUL !\n");
 	 printf("Waiting for MD5 checksum...\n");
     read(sockfd, rec, MD5_DIGEST_LENGTH);
     printf("\nRECEIVED !\n");
     int i, boolean = 1;
     for(i = 0; i<MD5_DIGEST_LENGTH;i++)
     	if(c[i]!=rec[i])
	  		 boolean =  0;

     if(boolean)
     	printf("MD5 checksum Matched !\n");
     else
     	printf("MD5 checksum Not Matched !\n");



  	// Close the socket
     
     close(sockfd);
     sleep(1);   
    
     return 0;
}
