#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <openssl/md5.h> 
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <bits/stdc++.h>


using namespace std;


#define BUFSIZE 1024
#define SLEEP_VAL 1


// struct for hello message
typedef struct{
int no_of_chunks;
int filesize;
float drop_probability;
char buf[1024];
}hello_msg;


int baseptr = -1;
int currptr = -1;



void error(string msg)
{
    perror(msg.c_str());
    exit(0);
}



void putHeader (char *msg, int seqNo, int size) {
    msg[3] = (seqNo >> 24) & 0xFF;
    msg[2] = (seqNo >> 16) & 0xFF;
    msg[1] = (seqNo >> 8) & 0xFF;
    msg[0] = (seqNo) & 0xFF;
    msg[7] = (size >> 24) & 0xFF;
    msg[6] = (size >> 16) & 0xFF;
    msg[5] = (size >> 8) & 0xFF;
    msg[4] = (size) & 0xFF;
}

void FindSeqNo (char *buf, int& ACKseqno) {
	ACKseqno = atoi(string(buf).c_str());
	
}



int main (int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
  	char* buffer;
  	FILE *fp;
  	hello_msg hello;

    if (argc != 3) {
       fprintf (stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket (AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname (hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }


      /* build the server's Internet address */
    bzero ((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy ((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);


    struct timeval tv;
	tv.tv_sec = SLEEP_VAL;
	tv.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(tv)) < 0) {
    	perror("Error");
	}   
   
     /* get a message from the user */

    // storing filename, filesize, no_of_chunks, drop-probability in hello_msg struct
    fflush(stdin);
    printf ("Please enter path of the file to be sent : ");
    scanf("%s", hello.buf);


    fp = fopen (hello.buf, "rb");

    if (fp == NULL) {
    	cout << "ERROR in reading file!" << endl;
    	return 0;
    }


    fseek(fp, 0L, SEEK_END);
	int sz = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	printf("The size of file is : %d \n", sz);
	hello.filesize = sz;
	hello.no_of_chunks = ceil(sz/1024.0);
	
	printf("Please enter the drop-probability at the receiver side : ");
	scanf("%f", &hello.drop_probability);


    // Sending hello_msg to the server
    serverlen = sizeof (serveraddr);
    n = sendto (sockfd, (void*)&hello, sizeof(hello_msg), 0, (const sockaddr *) &serveraddr, serverlen);
    if (n < 0) 
      error ("ERROR in sendto");
    
    /* print the server's reply */
  	buffer = (char*)malloc(sizeof(char)*BUFSIZE);
    n = recvfrom (sockfd, buffer, BUFSIZE, 0, (sockaddr *) &serveraddr, (socklen_t *) &serverlen);
    if (n < 0) 
      error ("ERROR in recvfrom");
    printf ("Echo from server: %s\n", buffer);


    printf("\n****Starting file transfer****\n\n");
    int bytesSent = 0;
    int filesize = sz;
    int CWnd = 3;
    int n2;
 	

    	while (bytesSent < filesize || currptr!=baseptr) 
    	{
    		if(currptr != hello.no_of_chunks-1)
    			bytesSent = 1024*(currptr + 1); 
    		else
    			bytesSent = filesize;

    		printf("\nCWnd = %d currptr = %d baseptr =%d \n",CWnd, currptr, baseptr);
    		printf("bytesSent = %d filesize =%d\n",bytesSent, filesize );
    		    	
    		while(baseptr - currptr < CWnd)
    		{

    			char msg[1032];
    			int n1, n2;
    			bzero (msg, 1032);
        		int nread = 0;

        		if(baseptr == hello.no_of_chunks-1)
        			break;
        		else{
        			baseptr++;
              fseek(fp, 1024*baseptr, SEEK_SET);
        		}
        		
        		if (filesize - 1024*baseptr < 1024) {       			
        			putHeader (msg, baseptr, filesize - 1024*baseptr);
            		nread = fread (msg + 8, 1, 1024, fp);
                	cout << "End of file reached!" << endl;
                	n1 = sendto (sockfd, msg, nread + 8, 0, (const sockaddr *) &serveraddr, serverlen);
            		cout << "Packet sent of size " << n1 << " bytes " << endl; 
    				if (n1 < 0) 
      					error ("ERROR in sendto");
        		}
        		else {
        			putHeader (msg, baseptr, 1024);
            		nread = fread (msg + 8, 1, 1024, fp);
            		n1 = sendto (sockfd, msg, nread + 8, 0, (const sockaddr *) &serveraddr, serverlen);
            		cout << "Packet sent of size " << n1 << " bytes " << endl; 
    				if (n1 < 0) 
      					error ("ERROR in sendto");
       			 }	
      		}


      		
      		while(1)
      		{
      		n2 = recvfrom (sockfd, buffer, BUFSIZE, 0, (sockaddr *) &serveraddr, (socklen_t *) &serverlen);
      		if (n2 < 0) 
      			break;                // TIMEOUT occurs

      		int ACKseqno;
      		FindSeqNo (buffer, ACKseqno);
      		CWnd += ACKseqno - currptr;
      		currptr = ACKseqno;
      		break;
      		}

      		
      		
      		if(n2<0){
      			baseptr = currptr;
      			if(CWnd > 1)
      				CWnd = CWnd/2;
      			else
      				CWnd =  1;
      		}     
       

        }    

 }


