#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <openssl/md5.h>


using namespace std;

#define BUFSIZE 10000

typedef struct{
int no_of_chunks;
int filesize;
float drop_probability;
char buf[1024];
}hello_msg;


/*
 * error - wrapper for perror
 */
void error(string msg) {
  perror(msg.c_str());
  exit(1);
}

void FindSeqNo (char *buf, int seqNo, int &recv) {
  char a[4];
  a[0] = buf[0];
  a[1] = buf[1];
  a[2] = buf[2];
  a[3] = buf[3];
  int sno = *(int *)a;
  recv = sno;
}


int ShouldReceive(float drop_probability, int sq)
{
	float f = 1.0*rand()/RAND_MAX;
	if(f<drop_probability){
    printf("Packet with sequence number %d dropped\n",sq);
		return 0;
  }
	else{
		printf("Packet with sequence number %d accepted\n",sq);
    return 1;
  }
}




int main(int argc, char **argv) {
  int sockfd; /* socket file descriptor - an ID to uniquely identify a socket by the application program */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */
  hello_msg hello;
  /* 
   * check command line arguments 
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port_for_server>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* 
   * socket: create the socket 
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
       (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* 
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1) {
    /*
     * recvfrom: receive a UDP datagram from a client
     */

    
    n = recvfrom(sockfd, (void*)&hello, sizeof(hello_msg), 0, (struct sockaddr *) &clientaddr, (socklen_t *) &clientlen);
    if (n < 0) {
      cout << "Hello" << endl;
      error("ERROR in recvfrom");
    }



    /* 
     * gethostbyaddr: determine who sent the datagram
     */
    
    printf("File Name : %s, File Size : %d bytes, No of chunks : %d, Drop-Probability : %f\n",hello.buf, hello.filesize, hello.no_of_chunks, hello.drop_probability);
    
    /* 
     * sendto: echo the input back to the client 
     */


    string hello_ack = "Received Hello Message !";
   
    n = sendto(sockfd, (char *) hello_ack.c_str(), hello_ack.size(), 0, 
	       (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0) 
      error("ERROR in sendto");


  

    string filename(hello.buf);
    int filesize = hello.filesize;

   
     printf("\n****Receiving File****\n\n");
    int bytesReceived = 0;
    int seqNo = 0;
    FILE *fp = fopen(filename.c_str() , "a");
    int n1, n2;

   


    while (bytesReceived < filesize) {
      
      	while(1) {
        int recvSeqNo;
       
        n1 = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, (socklen_t *) &clientlen);
        if (n1 < 0) {continue;}


        FindSeqNo(buf, seqNo, recvSeqNo);
       
        if(recvSeqNo == seqNo  && ShouldReceive(hello.drop_probability, recvSeqNo))
        {

          cout <<"Recv seq no : " << recvSeqNo << ", Received packet of " << n1 << " bytes! Total " << bytesReceived + n1 - 8 << endl;
          int repeatACKSeqNo = seqNo;
          
          n2 = sendto(sockfd, to_string(repeatACKSeqNo).c_str(), to_string(repeatACKSeqNo).size(), 0, 
            (struct sockaddr *) &clientaddr, clientlen);
          if (n2 < 0) cout << "ERROR in sending acknowlegement packet!" << endl;
          else cout << "Ack sent!" << endl;
          break;
        }

        // if recvfrom is not equal to seq then continue waiting for the same sequence number

        }

       	 int nwrite = fwrite (buf + 8, 1, n1 - 8, fp);
     	   cout << "Chars written " << nwrite << endl;
    	   bytesReceived += (n1 - 8);
         seqNo++;
         printf("\n");

    }




    cout << "File Transfer Complete!" << endl;
    fclose(fp);
  	return 0;
}


}
