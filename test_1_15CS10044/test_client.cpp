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


static int alarm_fired = 0;

// maintaining as global for use by mysig
int baseptr = 0;
int currptr = 0;



void mysig(int sig)
{
	pid_t pid;
	if (sig == SIGALRM)	
	{	
		if (baseptr - currptr == 3)
		alarm_fired = 1;
	}

}



void error(char *msg)
{
    perror(msg);
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
    char buf[BUFSIZE];

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


     /* get a message from the user */
    fflush(stdin);
    bzero (buf, BUFSIZE);
    printf ("Please enter path of the file to be sent : ");
    scanf("%s", buf);

    printf ("%s\n", buf);

    FILE *fp = fopen (buf, "rb");

    if (fp == NULL) {
    	cout << "ERROR in reading file!" << endl;
    	return 0;
    }


    fseek(fp, 0L, SEEK_END);
	int sz = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	printf("The size of file is : %d \n", sz);

	int no_of_chunks = sz/1024;

	char buffer[BUFSIZE];
    sprintf(buffer, "%s %d %d Hello", buf, sz, no_of_chunks);
    cout << string(buffer) << endl;


    // Sending filesize, number of chunks and filename to the server
    serverlen = sizeof (serveraddr);
    n = sendto (sockfd, buffer, strlen(buffer), 0, (const sockaddr *) &serveraddr, serverlen);
    if (n < 0) 
      error ("ERROR in sendto");
    
    /* print the server's reply */
    n = recvfrom (sockfd, buffer, BUFSIZE, 0, (sockaddr *) &serveraddr, (socklen_t *) &serverlen);
    if (n < 0) 
      error ("ERROR in recvfrom");
    printf ("Echo from server: %s", buf);


   

  

    int bytesSent = 0;
    int filesize = sz;
 



   

    	while (bytesSent < filesize) 
    	{

    		    	
    		while(baseptr - currptr <= 3)
    		{

    			char msg[1032];
    			int n1, n2;
    			bzero (msg, 1032);
    			if (filesize - bytesSent < 1024)
    				putHeader (msg, baseptr, filesize - bytesSent);
    			else 
    				putHeader (msg, baseptr, 1024);

        		int nread = 0;
        		if (filesize - bytesSent < 1024) {
            		while(!feof(fp)) { 
                		nread += fread (msg + 8, 1, 1024, fp);
            		}
            
            	if(feof(fp))
                	cout << "End of file reached!" << endl;
            
        		}
        		else {
            		nread = fread (msg + 8, 1, 1024, fp);
            
       			 }
    			
    			n1 = sendto (sockfd, msg, nread + 8, 0, (const sockaddr *) &serveraddr, serverlen);
            	cout << "Packet sent of size " << baseptr << " " << n1 << " bytes, total: " << bytesSent + n1 - 8 << "" << endl; 
    			if (n1 < 0) 
      				error ("ERROR in sendto");
      			baseptr++;
      			bytesSent += (n1 - 8);
      		}

      		
      		alarm_fired = 0;
      		alarm(SLEEP_VAL);
      		(void) signal(SIGALRM, mysig);

      		while(!alarm_fired)
      		{
      		int n2 = recvfrom (sockfd, buf, BUFSIZE, 0, (sockaddr *) &serveraddr, (socklen_t *) &serverlen);
      		if (n2 < 0) 
      			error ("ERROR in sendto");

      		int ACKseqno;
      		FindSeqNo (buf, ACKseqno);
      		currptr = ACKseqno;
      		printf("currptr = %d baseptr =%d\n",currptr, baseptr );
		break;
      		
      		}

      		//printf("currptr = %d baseptr =%d\n",currptr, baseptr );
      		
      		if(alarm_fired)
      			baseptr = currptr;      
       

        }
        
    

 }


