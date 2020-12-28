#include <stdlib.h> //exit(0);
#include <stdio.h> //myprint
#include <string.h> //memset
#include <unistd.h>
#include <math.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include "packet.h"

extern int printFlag ;

char* isLastToString (isLast i)
{
	switch (i)
	{
		case YES :
			return "YES" ;
		case NO :
			return "NO" ;
		default :
			return "islast_ERROR" ;
	}
}

char* packetTypeToString (packetType pkt)
{
	switch (pkt)
	{
		case DATA :
			return "DATA" ;
		case ACK :
			return "ACK" ;
		case CLOSE :
			return "CLOSE" ;
		default :
			return "packetType_ERROR" ;	
	}
}

int setSockAddr (struct sockaddr_in *server_addr, int relay_port)
{
	int sockfd, flags ;

	if ((sockfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		myprint ("Socket creation error\n") ;
        exit (0) ;
	}
 
    memset((char *) server_addr, 0, sizeof(struct sockaddr_in));
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(relay_port);
    server_addr->sin_addr.s_addr = inet_addr("127.0.0.1");

    flags = fcntl(sockfd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, flags);
     
	return sockfd ;
}

int setSockAddrBind (struct sockaddr_in *server_addr, int relay_port)
{
	int sockfd = setSockAddr (server_addr, relay_port) ;

    if (bind(sockfd, (struct sockaddr *)server_addr, sizeof(struct sockaddr_in))<0) 
	{ 
		perror("Bind failed"); 
		exit(EXIT_FAILURE); 
	}
     
	return sockfd ;
}

void printTime ()
{
    char buffer[15] ;
    int millisec;
    struct tm* tm_info;
    struct timeval tv;
    struct timespec spec;

    gettimeofday(&tv, NULL);
    clock_gettime(CLOCK_MONOTONIC, &spec);

    millisec = round(spec.tv_nsec / 1.0e6);
    if (millisec >= 999) 
    { 
	    millisec = 0 ;
	    tv.tv_sec++;
  	}

    tm_info = localtime(&tv.tv_sec);

    strftime(buffer, 15, "%H:%M:%S", tm_info);
    myprint(buffer, "%s.%03d", buffer, millisec) ;

    myprint ("%20s", buffer) ;
}

void printLog (char *nodeName, char *eventType, data *pkt, char *src, char *dest)
{
    myprint ("%10s%15s", nodeName, eventType) ;
    printTime () ;
    myprint ("%20s%10d%15s%15s", packetTypeToString (pkt->pktType), pkt->offset, src, dest) ;
    myprint ("\n") ;
}

void printHeading ()
{
	myprint ("\n%10s%15s%20s%20s%10s%15s%15s\n\n", "NODE NAME", "EVENT TYPE", "TIMESTAMP", "PACKET TYPE", "SEQ NO", "SOURCE",  "DESTINATION") ;
}
	
void printLine ()
{
	myprint ("\n-----------------------------------------------------------------------------------------------------------------\n") ;
	myprint ("-----------------------------------------------------------------------------------------------------------------\n") ;
}