#include "transport.cpp"


int main(int argc, char **argv)
{

    char buf[1024];
    int portno = atoi(argv[2]);
    char* ip = argv[1];
    init_transport_layer();

	

	
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

	bzero(buf,1024);
    sprintf(buf, "%d %s Hello", sz, filename);


    // Send filename file size
    appSend(buf, 1024, ip, portno);

    while(1)
    {

    	int nread = fread(buf,1,1024,fp);
    	printf("Bytes read %d \n", nread);
    	
    	// if read was success, send data
 		if(nread > 0)
 		{
 			printf("Sending... \n");
 			appSend(buf, nread, ip, portno);
 		}	   


		// nread < 256 indicates EOF 	
 		if(nread < 1024)
 		{
 			if(feof(fp))
 				printf("End of file\n");
 			if(ferror(fp))
 				printf("Error reading\n");
 			break;
 		}
    }
	
	pthread_join(receiver, NULL);
	pthread_join(rate_controller, NULL);

}
