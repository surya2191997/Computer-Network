#include "transport.cpp"


int main(int argc, char **argv)
{

	char buf[1025];

	int portno = atoi(argv[1]);
	char* ip;
	init_transport_layer();

	appRecv(buf, 1024, ip, portno);
	printf("%s\n",buf );

  
	// 2. Open the file and store the file size
     int k, j, i, len1, len2, fileSize, total=0, bytesReceived = 1;
     for(i = 0; i < 256; i++ )
     	if(buf[i]==' ')
     		break;

     len1 = i + 1;
     char size[len1];
     size[i] = '\0';
     for(j = 0 ; j < i ;j++)
     	size[j] = buf[j];
     sscanf(size, "%d", &fileSize);
    

  	for(j=i+1; j<256; j++)
  		if(buf[j]==' ')
  			break;

  	len2 = j - i;
  	char filename[len2];
  	filename[len2-1] = '\0';
  	for(k = i+1; k<j;k++)
  	  filename[k-i-1] = buf[k];


  
     FILE *fp;	
     fp = fopen(filename, "a");
     if(NULL == fp)
     {
     	printf("Error opening file");
     	return 1;
     }


	printf("*** Receiving File ***\n");


	while(total < fileSize)
  {
      bzero(buf, 1024);
      appRecv(buf,1024, ip, portno);

      if(fileSize - total >= 1024){
        fwrite(buf, 1, 1024, fp);
        total += 1024;
      }

      else{
        fwrite(buf, 1, fileSize - total, fp);
        total = fileSize;
      }
  }

      fclose(fp);


	pthread_join(receiver, NULL);
	pthread_join(rate_controller, NULL);

}