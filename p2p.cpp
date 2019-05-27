#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <bits/stdc++.h>
#include <arpa/inet.h>

using namespace std;

#define PORT 6789
#define TIMEOUT 1000


vector < vector<string> > user_info;

void make_user_table()
{
	vector<string> v;
	v.push_back("Nilay"); v.push_back("10.5.27.144"); v.push_back("6789");
	user_info.push_back(v);
	v.clear();
	v.push_back("Surya"); v.push_back("10.109.65.213"); v.push_back("6789");
	user_info.push_back(v);
}




int main()
{
	int i, master_socket, addrlen, peer_socket[5], max_peers = 5, optval;
    time_t last_activity[5];
	int sd, nread, max_sd, activity, new_socket;
	struct sockaddr_in address;
	char buffer[1025];
	fd_set readfds, writefds;

	make_user_table();
	for(i = 0 ; i < max_peers; i++)
		peer_socket[i] = 0;

	if((master_socket = socket(AF_INET, SOCK_STREAM, 0 )) == 0)
	{
		perror("Socket failed");
		exit(0);
	}	



    optval = 1;
    setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, 
       (const void *)&optval , sizeof(int));


	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

     if (listen(master_socket, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(address);
    struct timeval tv;
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
   

    while(1)
    {
        // re-initialize readfds
    	FD_ZERO(&readfds);
    	FD_SET(master_socket, &readfds);
    	FD_SET(0, &readfds);
    	max_sd = master_socket;
    	

    	for(i = 0 ; i < max_peers; i++)
    	{
    		sd = peer_socket[i];
    		if(sd > 0)
    			FD_SET(sd, &readfds);

    		if(sd > max_sd)
    			max_sd = sd;

    	}


        // if there is something read to be read, it will be written in readfds    	
    	activity = select(max_sd + 1, &readfds, NULL, NULL, &tv);
    	
        // if select returns nothing till timeout, application is close
        if(!activity)
    		break;


        // if the last_activity of any client was before the timeout duration,
        // close the corresponding socket

        
        for(i = 0 ; i < max_peers; i++)
            if(time(0) - last_activity[i] > TIMEOUT && peer_socket[i])
            {
                close(peer_socket[i]);
                peer_socket[i] = 0;
                
            }
        

    	
    	// CASE 1 : Activity occurs at the master_socket
        // Someone is trying to connect, so accept the connection
    	if(FD_ISSET(master_socket, &readfds))
    	{
    		if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }


            for(i = 0 ; i < max_peers; i++)
            {
            	if(peer_socket[i] == 0)
            	{
            		peer_socket[i] = new_socket;
                    last_activity[i] = time(0);
            		break;
            	}
            }
         }


        // CASE 2 : Activity occurs at the stdin
        // User wants to send the message
        if(FD_ISSET(0, &readfds))
        {
            // Get the name of host in hostname array from the message
           
            char hostname[100];
         	struct hostent *peer;
         	struct sockaddr_in peer_addr;
         	int boolean = 0, check = 0;
            memset(&buffer[0], 0, sizeof(buffer));
            fgets(buffer, 1024, stdin);
        
            for(i = 0 ; i < 1024; i++)
         	{
         	 	hostname[i] = buffer[i] ;
         	 	if(buffer[i] == '/')
         	  		break;
         	}
         	hostname[i] = '\0';   
         	string s(hostname);
            

            // Match the hostname in user_info table
         	for(i = 0 ; i < user_info.size(); i++)
         	{

         	  	if(s == user_info[i][0] || s == user_info[i][1])
				{	
					s = user_info[i][1];
					check = 1;
					break;         	 
				}
         	}    	

            
            // If there is no entry, print error
         	if(!check)
         	  	cout<<"Unknown Peer !"<<endl;


            // Else check if the host is already connected
         	else
         	{	  
         	  	peer = gethostbyname(s.c_str());
    		  	bzero((char *) &peer_addr, sizeof(peer_addr));
    		  	peer_addr.sin_family = AF_INET;
    		  	bcopy((char *)peer->h_addr, 
         	  	(char *)&peer_addr.sin_addr.s_addr,
         	  	peer->h_length);
    	      	peer_addr.sin_port = htons(PORT);

    		  
                // If yes, send the message
	          	for(i = 0 ; i < max_peers; i++)
	          	{
	          		if(peer_socket[i]!=0)
	          		{
	          			int res = getpeername(peer_socket[i], (struct sockaddr *)&address, (socklen_t*)&addrlen);
	          			if((address.sin_addr.s_addr == peer_addr.sin_addr.s_addr) && (address.sin_port == peer_addr.sin_port))
	          			{
	          			      boolean = 1;
	          			      write(peer_socket[i], buffer, 1024);  				
	          			      break;
	          			}
	          		}
	          	}

                // If not establish a connection, and then send a message
	          	if(!boolean)
	          	{
    		  		int new_socket =  socket(AF_INET, SOCK_STREAM, 0);
    		  		if (connect(new_socket,(struct sockaddr *)&peer_addr,sizeof(peer_addr)) < 0) 
        				cout<<"ERROR connecting"<<endl;

        	  		for(i = 0 ; i < max_peers; i++)
              		{
            			if(peer_socket[i] == 0)
            			{
            			     peer_socket[i] = new_socket;
            			     break;
            			}
              		}

              		write(new_socket, buffer, 1024);  		
                }

            }

	    }



		// CASE 3 : Activity occurs at one of the peers
        // 
		for(i = 0 ; i < max_peers; i++)
		{
		 	sd = peer_socket[i];
		 	if(FD_ISSET(sd, &readfds) && sd!=0)
		 	{
		 		if((nread = read(sd, buffer, 1024)) == 0)
		 		{
		 			// Somebody disconnected
		 			close(sd);
		 			peer_socket[i] = 0;
		 		}

		 		else
		 		{
                    // Print the message
                    last_activity[i] = time(0);

                    

                    char str[INET_ADDRSTRLEN];
                    int res = getpeername(peer_socket[i], (struct sockaddr *)&address, (socklen_t*)&addrlen);
                    inet_ntop(AF_INET, &(address.sin_addr), str, INET_ADDRSTRLEN);
                    
                    string s(str);
                    for(i = 0 ; i < user_info.size(); i++)
                        if(s == user_info[i][0] || s == user_info[i][1])
                            s = user_info[i][0];
                                   
                    cout<<s<<" : ";

                    for(i = 0 ; i < 1024; i++)                        
                        if(buffer[i] == '/')
                            break;

		 			buffer[nread] = '\0';
		 			printf("%s",buffer+i+1);		
		 		}
		 	}
		}		 
	}
}




        

    	



    	

   







t void *)&optval , sizeof(int));


	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

     if (listen(master_socket, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(address);
    struct timeval tv;
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
   

    while(1)
    {
        // re-initialize readfds
    	FD_ZERO(&readfds);
    	FD_SET(master_socket, &readfds);
    	FD_SET(0, &readfds);
    	max_sd = master_socket;
    	

    	for(i = 0 ; i < max_peers; i++)
    	{
    		sd = peer_socket[i];
    		if(sd > 0)
    			FD_SET(sd, &readfds);

    		if(sd > max_sd)
    			max_sd = sd;

    	}


        // if there is something read to be read, it will be written in readfds    	
    	activity = select(max_sd + 1, &readfds, NULL, NULL, &tv);
    	
        // if select returns nothing till timeout, application is close
        if(!activity)
    		break;


        // if the last_activity of any client was before the timeout duration,
        // close the corresponding socket

        
        for(i = 0 ; i < max_peers; i++)
            if(time(0) - last_activity[i] > TIMEOUT && peer_socket[i])
            {
                close(peer_socket[i]);
                peer_socket[i] = 0;
                
            }
        

    	
    	// CASE 1 : Activity occurs at the master_socket
        // Someone is trying to connect, so accept the connection
    	if(FD_ISSET(master_socket, &readfds))
    	{
    		if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }


            for(i = 0 ; i < max_peers; i++)
            {
            	if(peer_socket[i] == 0)
            	{
            		peer_socket[i] = new_socket;
                    last_activity[i] = time(0);
            		break;
            	}
            }
         }


        // CASE 2 : Activity occurs at the stdin
        // User wants to send the message
        if(FD_ISSET(0, &readfds))
        {
            // Get the name of host in hostname array from the message
           
            char hostname[100];
         	struct hostent *peer;
         	struct sockaddr_in peer_addr;
         	int boolean = 0, check = 0;
            memset(&buffer[0], 0, sizeof(buffer));
            fgets(buffer, 1024, stdin);
        
            for(i = 0 ; i < 1024; i++)
         	{
         	 	hostname[i] = buffer[i] ;
         	 	if(buffer[i] == '/')
         	  		break;
         	}
         	hostname[i] = '\0';   
         	string s(hostname);
            

            // Match the hostname in user_info table
         	for(i = 0 ; i < user_info.size(); i++)
         	{

         	  	if(s == user_info[i][0] || s == user_info[i][1])
				{	
					s = user_info[i][1];
					check = 1;
					break;         	 
				}
         	}    	

            
            // If there is no entry, print error
         	if(!check)
         	  	cout<<"Unknown Peer !"<<endl;


            // Else check if the host is already connected
         	else
         	{	  
         	  	peer = gethostbyname(s.c_str());
    		  	bzero((char *) &peer_addr, sizeof(peer_addr));
    		  	peer_addr.sin_family = AF_INET;
    		  	bcopy((char *)peer->h_addr, 
         	  	(char *)&peer_addr.sin_addr.s_addr,
         	  	peer->h_length);
    	      	peer_addr.sin_port = htons(PORT);

    		  
                // If yes, send the message
	          	for(i = 0 ; i < max_peers; i++)
	          	{
	          		if(peer_socket[i]!=0)
	          		{
	          			int res = getpeername(peer_socket[i], (struct sockaddr *)&address, (socklen_t*)&addrlen);
	          			if((address.sin_addr.s_addr == peer_addr.sin_addr.s_addr) && (address.sin_port == peer_addr.sin_port))
	          			{
	          			      boolean = 1;
	          			      write(peer_socket[i], buffer, 1024);  				
	t void *)&optval , sizeof(int));


	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

     if (listen(master_socket, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(address);
    struct timeval tv;
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
   

    while(1)
    {
        // re-initialize readfds
    	FD_ZERO(&readfds);
    	FD_SET(master_socket, &readfds);
    	FD_SET(0, &readfds);
    	max_sd = master_socket;
    	

    	for(i = 0 ; i < max_peers; i++)
    	{
    		sd = peer_socket[i];
    		if(sd > 0)
    			FD_SET(sd, &readfds);

    		if(sd > max_sd)
    			max_sd = sd;

    	}


        // if there is something read to be read, it will be written in readfds    	
    	activity = select(max_sd + 1, &readfds, NULL, NULL, &tv);
    	
        // if select returns nothing till timeout, application is close
        if(!activity)
    		break;


        // if the last_activity of any client was before the timeout duration,
        // close the corresponding socket

        
        for(i = 0 ; i < max_peers; i++)
            if(time(0) - last_activity[i] > TIMEOUT && peer_socket[i])
            {
                close(peer_socket[i]);
                peer_socket[i] = 0;
                
            }
        

    	
    	// CASE 1 : Activity occurs at the master_socket
        // Someone is trying to connect, so accept the connection
    	if(FD_ISSET(master_socket, &readfds))
    	{
    		if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }


            for(i = 0 ; i < max_peers; i++)
            {
            	if(peer_socket[i] == 0)
            	{
            		peer_socket[i] = new_socket;
                    last_activity[i] = time(0);
            		break;
            	}
            }
         }


        // CASE 2 : Activity occurs at the stdin
        // User wants to send the message
        if(FD_ISSET(0, &readfds))
        {
            // Get the name of host in hostname array from the message
           
            char hostname[100];
         	struct hostent *peer;
         	struct sockaddr_in peer_addr;
         	int boolean = 0, check = 0;
            memset(&buffer[0], 0, sizeof(buffer));
            fgets(buffer, 1024, stdin);
        
            for(i = 0 ; i < 1024; i++)
         	{
         	 	hostname[i] = buffer[i] ;
         	 	if(buffer[i] == '/')
         	  		break;
         	}
         	hostname[i] = '\0';   
         	string s(hostname);
            

            // Match the hostname in user_info table
         	for(i = 0 ; i < user_info.size(); i++)
         	{

         	  	if(s == user_info[i][0] || s == user_info[i][1])
				{	
					s = user_info[i][1];
					check = 1;
					break;         	 
				}
         	}    	

            
            // If there is no entry, print error
         	if(!check)
         	  	cout<<"Unknown Peer !"<<endl;


            // Else check if the host is already connected
         	else
         	{	  
         	  	peer = gethostbyname(s.c_str());
    		  	bzero((char *) &peer_addr, sizeof(peer_addr));
    		  	peer_addr.sin_family = AF_INET;
    		  	bcopy((char *)peer->h_addr, 
         	  	(char *)&peer_addr.sin_addr.s_addr,
         	  	peer->h_length);
    	      	peer_addr.sin_port = htons(PORT);

    		  
                // If yes, send the message
	          	for(i = 0 ; i < max_peers; i++)
	          	{
	          		if(peer_socket[i]!=0)
	          		{
	          			int res = getpeername(peer_socket[i], (struct sockaddr *)&address, (socklen_t*)&addrlen);
	          			if((address.sin_addr.s_addr == peer_addr.sin_addr.s_addr) && (address.sin_port == peer_addr.sin_port))
	          			{
	          			      boolean = 1;
	          			      write(peer_socket[i], buffer, 1024);  				
	          			      break;
	          			}
	          		}
	          	}

                // If not establish a connection, and then send a message
	          	if(!boolean)
	          	{
    		  		int new_socket =  socket(AF_INET, SOCK_STREAM, 0);
    		  		if (connect(new_socket,(struct sockaddr *)&peer_addr,sizeof(peer_addr)) < 0) 
        				cout<<"ERROR connecting"<<endl;

        	  		for(i = 0 ; i < max_peers; i++)
              		{
            			if(peer_socket[i] == 0)
            			{
            			     peer_socket[i] = new_socket;
            			     break;
            			}
              		}

              		write(new_socket, buffer, 1024);  		
                }

            }

	    }



		// CASE 3 : Activity occurs at one of the peers
        // 
		for(i = 0 ; i < max_peers; i++)
		{
		 	sd = peer_socket[i];
		 	if(FD_ISSET(sd, &readfds) && sd!=0)
		 	{
		 		if((nread = read(sd, buffer, 1024)) == 0)
		 		{
		 			// Somebody disconnected
		 			close(sd);
		 			peer_socket[i] = 0;
		 		}

		 		else
		 		{
                    // Print the message
                    last_activity[i] = time(0);

                    

                    char str[INET_ADDRSTRLEN];
                    int res = getpeername(peer_socket[i], (struct sockaddr *)&address, (socklen_t*)&addrlen);
                    inet_ntop(AF_INET, &(address.sin_addr), str, INET_ADDRSTRLEN);
                    
                    string s(str);
                    for(i = 0 ; i < user_info.size(); i++)
                        if(s == user_info[i][0] || s == user_info[i][1])
                            s = user_info[i][0];
                                   
                    cout<<s<<" : ";

                    for(i = 0 ; i < 1024; i++)                        
                        if(buffer[i] == '/')
                            break;

		 			buffer[nread] = '\0';
		 			printf("%s",buffer+i+1);		
		 		}
		 	}
		}		 
	}
}




        

    	



    	

   







t void *)&optval , sizeof(int));


	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

     if (listen(master_socket, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(address);
    struct timeval tv;
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
   

    while(1)
    {
        // re-initialize readfds
    	FD_ZERO(&readfds);
    	FD_SET(master_socket, &readfds);
    	FD_SET(0, &readfds);
    	max_sd = master_socket;
    	

    	for(i = 0 ; i < max_peers; i++)
    	{
    		sd = peer_socket[i];
    		if(sd > 0)
    			FD_SET(sd, &readfds);

    		if(sd > max_sd)
    			max_sd = sd;

    	}


        // if there is something read to be read, it will be written in readfds    	
    	activity = select(max_sd + 1, &readfds, NULL, NULL, &tv);
    	
        // if select returns nothing till timeout, application is close
        if(!activity)
    		break;


        // if the last_activity of any client was before the timeout duration,
        // close the corresponding socket

        
        for(i = 0 ; i < max_peers; i++)
            if(time(0) - last_activity[i] > TIMEOUT && peer_socket[i])
            {
                close(peer_socket[i]);
                peer_socket[i] = 0;
                
            }
        

    	
    	// CASE 1 : Activity occurs at the master_socket
        // Someone is trying to connect, so accept the connection
    	if(FD_ISSET(master_socket, &readfds))
    	{
    		if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }


            for(i = 0 ; i < max_peers; i++)
            {
            	if(peer_socket[i] == 0)
            	{
            		peer_socket[i] = new_socket;
                    last_activity[i] = time(0);
            		break;
            	}
            }
         }


        // CASE 2 : Activity occurs at the stdin
        // User wants to send the message
        if(FD_ISSET(0, &readfds))
        {
            // Get the name of host in hostname array from the message
           
            char hostname[100];
         	struct hostent *peer;
         	struct sockaddr_in peer_addr;
         	int boolean = 0, check = 0;
            memset(&buffer[0], 0, sizeof(buffer));
            fgets(buffer, 1024, stdin);
        
            for(i = 0 ; i < 1024; i++)
         	{
         	 	hostname[i] = buffer[i] ;
         	 	if(buffer[i] == '/')
         	  		break;
         	}
         	hostname[i] = '\0';   
         	string s(hostname);
            

            // Match the hostname in user_info table
         	for(i = 0 ; i < user_info.size(); i++)
         	{

         	  	if(s == user_info[i][0] || s == user_info[i][1])
				{	
					s = user_info[i][1];
					check = 1;
					break;         	 
				}
         	}    	

            
            // If there is no entry, print error
         	if(!check)
         	  	cout<<"Unknown Peer !"<<endl;


            // Else check if the host is already connected
         	else
         	{	  
         	  	peer = gethostbyname(s.c_str());
    		  	bzero((char *) &peer_addr, sizeof(peer_addr));
    		  	peer_addr.sin_family = AF_INET;
    		  	bcopy((char *)peer->h_addr, 
         	  	(char *)&peer_addr.sin_addr.s_addr,
         	  	peer->h_length);
    	      	peer_addr.sin_port = htons(PORT);

    		  
                // If yes, send the message
	          	for(i = 0 ; i < max_peers; i++)
	          	{
	          		if(peer_socket[i]!=0)
	          		{
	          			int res = getpeername(peer_socket[i], (struct sockaddr *)&address, (socklen_t*)&addrlen);
	          			if((address.sin_addr.s_addr == peer_addr.sin_addr.s_addr) && (address.sin_port == peer_addr.sin_port))
	          			{
	          			      boolean = 1;
	          			      write(peer_socket[i], buffer, 1024);  				
	t void *)&optval , sizeof(int));


	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

     if (listen(master_socket, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(address);
    struct timeval tv;
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
   

    while(1)
    {
        // re-initialize readfds
    	FD_ZERO(&readfds);
    	FD_SET(master_socket, &readfds);
    	FD_SET(0, &readfds);
    	max_sd = master_socket;
    	

    	for(i = 0 ; i < max_peers; i++)
    	{
    		sd = peer_socket[i];
    		if(sd > 0)
    			FD_SET(sd, &readfds);

    		if(sd > max_sd)
    			max_sd = sd;

    	}


        // if there is something read to be read, it will be written in readfds    	
    	activity = select(max_sd + 1, &readfds, NULL, NULL, &tv);
    	
        // if select returns nothing till timeout, application is close
        if(!activity)
    		break;


 