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
#include <pthread.h>


using namespace std;

# define MAX_CONNECTIONS 1
# define BUFSIZE 65536
# define MSS 1024
# define TIMEOUT 1

typedef struct{
	int no_of_bytes;
	int sqNo;
	int type;
	int AdvWinSize;
	char buffer[1024];
}Packet;

void* receive_from(void *);
void* send_to(void *);
void* rate_control(void *);
void create_packet();



pthread_mutex_t SendQ_lock, sendBuffer_lock, recvBuffer_lock;
pthread_t parent, receiver, rate_controller, timer;
int first_byte_sqNo = 0, global_sqNo = 0, no_of_DUPACKS = -1, prevACKsqNo, last_byte_sqNo_rcv = -1;
int Wnd = MSS, CWnd = MSS, ssthresth = 100*MSS;
int baseptr = -1;
int timeout = 0, t;
float curr_time, startTime;
queue< char > sendBuffer;
deque< char > recvBuffer;
deque< Packet > SendQ;
Packet curr_packet, recv_packet;
int sockfd = 0;
struct sockaddr_in serveraddr, cliaddr;
int serverlen, clilen;


void error(string msg)
{
    perror(msg.c_str());
    exit(0);
}

void debug(string msg)
{
    perror(msg.c_str());
}

void sig_handle(int signo)
{
	if(signo == SIGUSR1)
	{}
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





void init_transport_layer()
{
	/*	Separate threads for send, receive and 
	 *	rate control operations
	 */
	 
	/*	Initialze the buffers and Connection Table */
	signal(SIGUSR1, sig_handle);
	serverlen = sizeof(serveraddr);
	clilen = sizeof(cliaddr);

	parent = pthread_self();
	
	pthread_create(&receiver, NULL, receive_from, NULL);
	pthread_create(&rate_controller, NULL, rate_control, NULL);	
}


void update_window()
{
	// update packet queue to delete all acknowledged packets
	// and store only the outstanding ones

	int i = 0;
	if(SendQ.size())
	{
		while(SendQ.at(i).sqNo <= recv_packet.sqNo)
		{
			first_byte_sqNo = SendQ.back().sqNo;
			SendQ.pop_front();
			i++;baseptr--;
			if(i >= SendQ.size())
				break;
		}
	}
	

	// timeout occured or 3 Dupacks received
	if(timeout || no_of_DUPACKS == 3)
	{
		ssthresth = CWnd/2;
		if(timeout)       // timeout
			CWnd = MSS;
		else				 // DUPACKS
			CWnd = ssthresth;
		Wnd = CWnd;
		baseptr = -1;
	}

	// ACK received or Sending initial packet
	else
	{
		// slow start
		if(CWnd < ssthresth)
			CWnd += i*MSS;
		// AIMD
		else
			CWnd += MSS/CWnd;

		Wnd = min(CWnd, recv_packet.AdvWinSize);				
	}

	if(timeout)
		printf("Timeout occured !\n");
	printf("update_window() called  : Wnd = %d \n", Wnd);	
	pthread_kill(rate_controller, SIGUSR1);
	cout<<"Wnd - baseptr = "<<baseptr<<" size = "<<SendQ.size()<<endl;
}


void* timer_thread(void*)
{
	time_t startTime = time(0);
	while(time(0) - startTime < TIMEOUT)
		curr_time = time(0) - startTime;

	curr_time = 0;
	timeout = 1;
	update_window();
	
}	



void* rate_control(void *)
{
	
	while(1)
	{
		
		while(baseptr < ceil(Wnd/MSS)-1)
		{	
			pthread_mutex_lock(&SendQ_lock);

			pthread_mutex_lock(&sendBuffer_lock);
			int siz = sendBuffer.size();
			pthread_mutex_unlock(&sendBuffer_lock);



			if(baseptr == SendQ.size() - 1)
			{
				if(siz)
					create_packet();
				else{
					pthread_mutex_unlock(&SendQ_lock); 
					break;
				}
			}

			
			baseptr++;
			//cout<<"baseptr = "<<baseptr<<" size = "<<SendQ.size()<<endl;
			Packet p = SendQ.at(baseptr);
			int n = sendto(sockfd, (void*)&p, sizeof(Packet), 0, (const sockaddr*)&serveraddr, serverlen);
			printf("DATA packet of size %d bytes sent (%d)\n", p.no_of_bytes, p.sqNo );

			pthread_mutex_unlock(&SendQ_lock);
		}

		//if(sendBuffer.size() == 0)
		//	pause();

	}
}


void create_packet()
{
	// create packet

	pthread_mutex_lock(&sendBuffer_lock);
	Packet p;
	int i = 0;

	if(sendBuffer.size() >= MSS)
	{	

		p.no_of_bytes = 1024;
		while(i < 1024)
		{
			p.buffer[i] = sendBuffer.front();
			sendBuffer.pop();
			i++;
			global_sqNo++;
		}
	}

	else
	{
		p.no_of_bytes = sendBuffer.size();
		while(sendBuffer.size())
		{
			p.buffer[i] = sendBuffer.front();
			sendBuffer.pop();
			i++;
			global_sqNo++;
		}
	}
	
	pthread_mutex_unlock(&sendBuffer_lock);

	p.sqNo = global_sqNo-1;
	p.type = 1;
	p.AdvWinSize = 0;

	SendQ.push_back(p);
	pthread_kill(parent, SIGUSR1);
}

	



int parse_packet(Packet p)
{
	return p.type;
}



void* receive_from(void *)
{
	while(1){
		if(sockfd)
		{
			timeout = 0;
			int n = recvfrom (sockfd, (void*)&recv_packet, sizeof(Packet), 0, (sockaddr *) &cliaddr, (socklen_t *) &clilen);

			if(n<0)
			{
				pthread_mutex_lock(&SendQ_lock);
				timeout = 1;
				update_window();	
				pthread_mutex_unlock(&SendQ_lock);
				continue;
			}


			else if(parse_packet(recv_packet))
			{
				// DATA received
				if(recv_packet.sqNo - recv_packet.no_of_bytes <= last_byte_sqNo_rcv  )
				{
					if(recv_packet.no_of_bytes <= BUFSIZE - recvBuffer.size())
					{
						pthread_mutex_lock(&recvBuffer_lock);
						if(ShouldReceive(0, recv_packet.sqNo))
						{
							last_byte_sqNo_rcv = recv_packet.sqNo;
							int i = 0;

							
							
							while(i <= recv_packet.no_of_bytes)
								recvBuffer.push_back(recv_packet.buffer[i++]);

							Packet p;
							p.no_of_bytes = 0; p.type = 0; p.AdvWinSize = BUFSIZE - recvBuffer.size();
							

							if(recv_packet.sqNo - recv_packet.no_of_bytes == last_byte_sqNo_rcv)
								p.sqNo = recv_packet.sqNo;
							else
								p.sqNo = last_byte_sqNo_rcv;

							int n = sendto(sockfd, (void*)&p, sizeof(Packet), 0, (const sockaddr*)&cliaddr, clilen);			
							printf("ACK packet sent (%d)\n",  p.sqNo);
						}
						pthread_mutex_unlock(&recvBuffer_lock);
					}
				}

				// out of order packet received
				else
				{
					if(recv_packet.sqNo - last_byte_sqNo_rcv < BUFSIZE - recvBuffer.size())
					{
						int i = last_byte_sqNo_rcv + 1, k = 0;
						pthread_mutex_lock(&recvBuffer_lock);
						while(i <= recv_packet.sqNo)
							if(i < recv_packet.sqNo - recv_packet.no_of_bytes -1)
								recvBuffer.push_back(0);
							else
								recvBuffer.push_back(recv_packet.buffer[k++]);

						Packet p;
						p.no_of_bytes = 0; p.type = 0; p.AdvWinSize = BUFSIZE - recvBuffer.size();
						int n = sendto(sockfd, (void*)&p, sizeof(Packet), 0, (const sockaddr*)&cliaddr, clilen);			
						printf("ACK packet sent (out of order) (%d)\n",  p.sqNo);

					}
				}

			}

			else
			{		
				pthread_mutex_lock(&SendQ_lock);
				printf("ACK received!! %d\n", recv_packet.sqNo);
				if(recv_packet.sqNo == prevACKsqNo)
				{
					if(no_of_DUPACKS == 3)
						no_of_DUPACKS = -1;
					no_of_DUPACKS++;
				}
				prevACKsqNo = recv_packet.sqNo;
				timeout = 0;
				update_window();	
				pthread_mutex_unlock(&SendQ_lock);
			}
			pthread_kill(parent, SIGUSR1);
		}
	}
}

void sendbuffer_handler(char* buf, size_t len)
{
	int i = 0;
	while(1)
	{
		pthread_mutex_lock(&sendBuffer_lock);
		if(len < BUFSIZE - sendBuffer.size())
		{	
			while(i != len)
			{
				sendBuffer.push(buf[i]);
				i++;
			}
			pthread_mutex_unlock(&sendBuffer_lock);
			pthread_kill(rate_controller, SIGUSR1);
			break;
		}
		else{
			pthread_mutex_unlock(&sendBuffer_lock);
			pause();
		}
	}
	

}


void recvbuffer_handler(char* buf, size_t len)
{
	int i = 0;

	while(1)
	{
		pthread_mutex_lock(&recvBuffer_lock);
		
		if(recvBuffer.size())
		{
			while(i<=len && recvBuffer.size())
			{
				buf[i] = recvBuffer.front();
				recvBuffer.pop_front();
				i++;
			}
			buf[len] = '\0';
			pthread_mutex_unlock(&recvBuffer_lock);
			break;

		}

		else
		{
			pthread_mutex_unlock(&recvBuffer_lock);
			pause();
		}
	}
}


void appSend(char* buf, size_t len, char* ip, int port)
{
	if(!sockfd)
	{		
		sockfd = socket (AF_INET, SOCK_DGRAM, 0);
    	if (sockfd < 0) 
        	error("ERROR opening socket");

       

        struct timeval tv;
		tv.tv_sec = TIMEOUT;
		tv.tv_usec = 0;
		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(tv)) < 0) {
    	perror("Error");
		}   

		

    	/* gethostbyname: get the server's DNS entry */
    	struct hostent *server = gethostbyname (ip);

    	if (server == NULL) {
        	fprintf(stderr,"ERROR, no such host as %s\n", ip);
        	exit(0);
    	}

    
        /* build the server's Internet address */
    	bzero ((char *) &serveraddr, sizeof(serveraddr));
    	serveraddr.sin_family = AF_INET;
    	bcopy ((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    	serveraddr.sin_port = htons(port);

    }

	sendbuffer_handler(buf, len);
}



void appRecv(char* buf, size_t len, char* ip, int port)
{
  if(!sockfd)

  {
  	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  	if (sockfd < 0) 
   		error("ERROR opening socket");

  	int optval = 1;
  	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
       (const void *)&optval , sizeof(int));

  
  	bzero((char *) &serveraddr, sizeof(serveraddr));
  	serveraddr.sin_family = AF_INET;
  	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  	serveraddr.sin_port = htons((unsigned short)port);

  	struct timeval tv;
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(tv)) < 0) {
    	perror("Error");
	}   

	if (bind(sockfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");
   }

   	recvbuffer_handler(buf, len);
}
