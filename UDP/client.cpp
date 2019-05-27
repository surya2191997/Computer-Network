#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <bits/stdc++.h>
#include <openssl/md5.h>

#define BUFSIZE 1024

using namespace std;

void error(char *msg) {
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

bool chkSeqNo (char *buf, int seqNo) {
	if (atoi(string(buf).c_str()) == seqNo) return true;
	return false;
}

void parseFileNameSize (string toParse, string &filename, int &filesize) {
	istringstream ss (toParse);
	string temp;
	vector<string> v;
	while (ss >> temp) {
		v.push_back (temp);
	}

	if (v.size() != 2) exit(0);
	else {
		filename = v[0];
		cout << "Filename: " << filename << endl;
		filesize = atoi(v[1].c_str());
		cout << "Filesize: " << filesize << endl;
	}
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

    struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(tv)) < 0) {
    	perror("Error");
	}

    /* build the server's Internet address */
    bzero ((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy ((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* get a message from the user */
    fflush(stdin);
    bzero (buf, BUFSIZE);
    printf ("Please enter path and size of the file to be sent (seperated by a space): ");
    fgets (buf, BUFSIZE, stdin);
    cout << string(buf) << endl;
    /* send the message to the server */
    serverlen = sizeof (serveraddr);
    n = sendto (sockfd, buf, strlen(buf), 0, (const sockaddr *) &serveraddr, serverlen);
    if (n < 0) 
      error ("ERROR in sendto");
    
    /* print the server's reply */
    n = recvfrom (sockfd, buf, BUFSIZE, 0, (sockaddr *) &serveraddr, (socklen_t *) &serverlen);
    if (n < 0) 
      error ("ERROR in recvfrom");
    printf ("Echo from server: %s", buf);

    unsigned char c[MD5_DIGEST_LENGTH];
    MD5_CTX mdContext;
    MD5_Init (&mdContext);

    string filename;
    int filesize;

    parseFileNameSize (string(buf), filename, filesize);
    cout << filename << endl;
    FILE *fp = fopen (filename.c_str(), "rb");
    if (fp == NULL) {
    	cout << "ERROR in reading file!" << endl;
    	return 0;
    }
    fseek(fp, 0L, SEEK_END);
	int sz = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	printf("The size of file is : %d \n", sz);

    int bytesSent = 0;
    int seqNo = 0;

    while (bytesSent < filesize) {
    	char msg[1032];
    	int n1, n2;
    	bzero (msg, 1032);
    	if (filesize - bytesSent < 1024)
    		putHeader (msg, seqNo, filesize - bytesSent);
    	else 
    		putHeader (msg, seqNo, 1024);

        int nread = 0;
        if (filesize - bytesSent < 1024) {
            while(!feof(fp)) { 
                nread += fread (msg + 8, 1, 1024, fp);
            }
            MD5_Update (&mdContext, msg + 8, nread);
            if(feof(fp))
                cout << "End of file reached!" << endl;
            MD5_Final (c,&mdContext);
        }
        else {
            nread = fread (msg + 8, 1, 1024, fp);
            MD5_Update (&mdContext, msg + 8, nread);
        }
    	while (1) {
    		n1 = sendto (sockfd, msg, nread + 8, 0, (const sockaddr *) &serveraddr, serverlen);
            cout << "Packet sent of size " << seqNo << " " << n1 << " bytes, total: " << bytesSent + n1 - 8 << "" << endl; 
    		if (n1 < 0) 
      			error ("ERROR in sendto");
      		n2 = recvfrom (sockfd, buf, BUFSIZE, 0, (sockaddr *) &serveraddr, (socklen_t *) &serverlen);
      		if (n2 < 0) {
      			cout << "TIMEOUT!" << endl;
      			continue;
      		}
      		if (chkSeqNo (buf, seqNo)) {
      			cout << "ACK received!" << endl;
      			break;
      		}
      		else cout << "WRONG ACK received!" << endl;
        }
        
     	bytesSent += (n1 - 8);
     	seqNo++;
    }
    sleep(2);
    unsigned char c2[MD5_DIGEST_LENGTH];
    int n2 = recvfrom (sockfd, c2, MD5_DIGEST_LENGTH, 0, (sockaddr *) &serveraddr, (socklen_t *) &serverlen);
    
    cout << "Checksums!" << endl;
    for(int i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", c[i]); 
    cout << endl;
    for(int i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", c2[i]);
    return 0;
}
