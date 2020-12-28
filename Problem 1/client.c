#include<stdlib.h> //exit(0);
#include<stdio.h> //myprint
#include<string.h> //memset
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include <sys/wait.h>
#include "packet.h"

#define BUFLEN 2  //Max length of buffer
#define SERVER_PORT 8888  // The port on which to send data

void die(char *s)
{
    perror(s);
    exit(1);
}

int setServer (struct sockaddr_in *server_addr, fd_set* readfds)
{
	int sockfd ;

	if ((sockfd=socket(AF_INET, SOCK_STREAM, 0)) == -1)
        die("socket\n");

    FD_ZERO(readfds); 
    FD_SET (sockfd, readfds) ;
 
    memset((char *) server_addr, 0, sizeof(struct sockaddr_in));
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(SERVER_PORT);
    server_addr->sin_addr.s_addr = inet_addr("127.0.0.1");
     
    if (connect (sockfd, (struct sockaddr *)server_addr, sizeof(struct sockaddr_in)) < 0)
    	die ("Connection even failed!\n") ;

	return sockfd ;
}

// resendPrep (&readfds, sockfd, &timeout, &sndCount) ;

int resendPrep (fd_set *readfds, int sockfd, struct timeval *timeout, int *sndCount)
{
    (*sndCount)++ ;
	FD_ZERO(readfds); 
	FD_SET (sockfd, readfds) ;
	timeout->tv_sec = TIMEOUT ;
	timeout->tv_usec = 0 ;
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf ("Invalid number of arguments! ./client 1 for printing enabled\n") ;
		exit (0) ;
	}

	int printFlag ;
	if (!strcmp(argv[1], "1"))
		printFlag = 1 ;
	else if (!strcmp(argv[1], "0"))
		printFlag = 0 ;
	else
	{
		printf ("Print flag is incorrect. Specify as true or else\n") ;
		exit (0) ;
	}

	/* ------------------------------------------------------------------------------------------------- */


	struct sockaddr_in server_addr;
    int sockfd, i, slen = sizeof(server_addr), sndCount, fileSize, noPkts, bytesRead ;
    struct timeval timeout = {TIMEOUT,0} ;
    fd_set readfds ;

    data *datBuf, *ackPkt ;
	data *datCache [10] ;
	ackPkt = (data *) malloc (sizeof(data)) ;
	ackPkt->pktType = DATA ;

	FILE *fp = fopen ("input.txt", "r") ;

	fseek (fp, 0, SEEK_END) ;
	fileSize = ftell (fp) - 1 ;
	noPkts = fileSize/PACKET_SIZE + ((fileSize % PACKET_SIZE)?1:0) ;
	fseek (fp, 0, SEEK_SET) ;

	myprint ("Size of file = %d\n", fileSize) ;
	myprint ("No of packets = %d\n", noPkts) ;

	if (fork())
	{
		sockfd = setServer (&server_addr, &readfds) ;

	    for (int i = 0 ; i < noPkts ; i += 2)
	    {
	    	datBuf = (data *) malloc (sizeof(data)) ;
	    	datBuf->offset = i*PACKET_SIZE ;
	    	datBuf->last = (i == noPkts-1)?YES:NO ;
	    	datBuf->pktType = DATA ;
	    	datBuf->channel = EVEN ;

	    	fseek (fp, i*PACKET_SIZE, SEEK_SET) ;
	    	bytesRead = (int)fread (datBuf->stuff , sizeof(char), PACKET_SIZE, fp) ;
	    	datBuf->stuff[bytesRead] = '\0' ;
	    	datBuf->payload = bytesRead ;
	    	datCache[i] = datBuf ;

	    	sndCount = 1 ;
	        while (1)
	        {
	        	send (sockfd, (char *)datBuf, sizeof(data), 0) ;
	        	myprint ("SENT PKT : Seq No %d of size %d bytes from channel %d\n", datBuf->offset, datBuf->payload, datBuf->channel) ;
	        	select (sockfd + 1, &readfds, NULL, NULL, &timeout) ;

	        	if (FD_ISSET (sockfd, &readfds))
	        	{
	        		read (sockfd, (char *)ackPkt, sizeof(data)) ;
	        		myprint ("RCVD ACK : for PKT with Seq No %d from channel %d\n", ackPkt->offset, ackPkt->channel) ;
	        		break ;	
	        	}

	        	resendPrep (&readfds, sockfd, &timeout, &sndCount) ;

	        }
	    }

	    wait (NULL) ;
	    fclose (fp) ;
	    close(sockfd);
	}
	else
	{
		sockfd = setServer (&server_addr, &readfds) ;

	    for (int i = 1 ; i < noPkts ; i += 2)
	    {
	    	datBuf = (data *) malloc (sizeof(data)) ;
	    	datBuf->offset = i*PACKET_SIZE ;
	    	datBuf->last = (i == noPkts-1)?YES:NO ;
	    	datBuf->pktType = DATA ;
	    	datBuf->channel = ODD ;
	    	
	    	fseek (fp, i*PACKET_SIZE, SEEK_SET) ;
	    	bytesRead = (int)fread (datBuf->stuff , sizeof(char), PACKET_SIZE, fp) ;
	    	datBuf->stuff[bytesRead] = '\0' ;
	    	datBuf->payload = bytesRead ;
	    	datCache[i] = datBuf ;

	    	
	    	sndCount = 1 ;
	        while (1)
	        {
	        	send (sockfd, (char *)datBuf, sizeof(data), 0) ;
	        	myprint ("SENT PKT : Seq No %d of size %d bytes from channel %d\n", datBuf->offset, datBuf->payload, datBuf->channel) ;
	        	select (sockfd + 1 , &readfds, NULL, NULL, &timeout) ;

	        	if (FD_ISSET (sockfd, &readfds))
	        	{
	        		read (sockfd, (char *)ackPkt, sizeof(data)) ;
	        		myprint ("RCVD ACK : for PKT with Seq No %d from channel %d\n", ackPkt->offset, ackPkt->channel) ;
	        		break ;	
	        	}

				resendPrep (&readfds, sockfd, &timeout, &sndCount) ;
	        }
	    }
	    close(sockfd);
	}
	
	exit (0) ;
}
