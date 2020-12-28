#include <stdio.h> 
#include <string.h> //strlen 
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h> //close 
#include <arpa/inet.h> //close 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 
#include <time.h> 
#include <fcntl.h>
#include "packet.h"
	
#define TRUE 1 
#define FALSE 0 

int printFlag ;
	
void initRecvOffsets (int *recvOffsets)
{
	int i ;

	for (i = 0 ; i < WINDOW_SIZE ; i++)
		recvOffsets[i] = -1 ;
} 

void updateRecvOffsets (int *recvOffsets, int offset)
{
	int min = recvOffsets[0], minPos = 0, max = recvOffsets[0], i ;

	for (i = 1 ; i < WINDOW_SIZE ; i++)
	{
		if (recvOffsets[i] < min)
		{
			min = recvOffsets[i] ;
			minPos = i ;
		}

		if (recvOffsets[i] > max)
			max = recvOffsets[i] ;
	}

	if (offset > max || offset > min)
		recvOffsets[minPos] = offset ; 
}

int lastWindowAllRecv (int *recvOffsets, int lastOffset)
{
	int i, j, srch = lastOffset, srchFlag = 0 ;

	for (i = 0 ; i < WINDOW_SIZE ; i++)
	{
		srchFlag = 0 ;
		for (j = 0 ; j < WINDOW_SIZE ; j++)
		{
			if (recvOffsets[j] == srch)
			{
				srchFlag = 1 ;
				break ;
			}
		}

		if (!srchFlag)
			return 0 ;

		srch = srch - PACKET_SIZE ;
	}

	return 1 ;
}

int main(int argc , char *argv[]) 
{ 
	if (argc != 3)
	{
		printf ("Invalid number of arguments. ./server 0.45 1 for drop probability 0.45 and printing enabled\n") ;
		exit (0) ;
	}

	double DROP = atof (argv[1]) ;
	if (DROP < 0.0 || DROP >= 1.0)
	{
		printf ("Please specify drop probability between 0 and 1\n") ;
		exit (0) ;
	}

	if (!strcmp(argv[2], "1"))
		printFlag = 1 ;
	else if (!strcmp(argv[2], "0"))
		printFlag = 0 ;
	else
	{
		printf ("Print flag is incorrect. Specify as true or else\n") ;
		exit (0) ;
	}


	/* ------------------------------------------------------------------------------------------------------- */
	struct sockaddr_in serverAddr, relayEvenAddr, relayOddAddr, otherAddr ;
	int serverSock, relayEvenSock, relayOddSock, valRead, disconnected = 0, i, lastFlag = 0, lastOffset, pktNo ;
	int slen = sizeof (struct sockaddr_in) ;
	int recvOffsets[WINDOW_SIZE] ;

	initRecvOffsets (recvOffsets) ;

	data *datPkt = (data *) malloc (sizeof (data)) ;
	data *ackPkt = (data *) malloc (sizeof (data)) ;
	ackPkt->pktType = ACK ;

	serverSock = setSockAddrBind (&serverAddr, SERVER_PORT) ;
	relayEvenSock = setSockAddr (&relayEvenAddr, RELAY_EVEN_PORT) ;
	relayOddSock = setSockAddr (&relayOddAddr, RELAY_ODD_PORT) ;

	FILE *fp ;
	fp = fopen ("output.txt", "w") ;

	fd_set serverfd ;
	FD_ZERO (&serverfd) ;
	FD_SET (serverSock, &serverfd) ;

	myprint ("Waiting for connection!\n") ;
	printLine () ;
	printHeading () ;

	double time ;
	struct timeval start, end ;
	gettimeofday (&start, NULL) ;

	while (TRUE)
	{
		select (serverSock + 1 , &serverfd, NULL, NULL, NULL) ;
		if (FD_ISSET(serverSock, &serverfd))
		{
			while (recvfrom(serverSock , datPkt, sizeof(data), 0, (struct sockaddr *) &otherAddr, &slen) != -1)
			{
				pktNo = datPkt->offset / PACKET_SIZE ;
				printLog ("SERVER", "R", datPkt, pktNo % 2 ? "RELAY_ODD" : "RELAY_EVEN", "SERVER") ;

				updateRecvOffsets (recvOffsets, datPkt->offset) ;

				fseek (fp, datPkt->offset, SEEK_SET) ;
				fwrite (datPkt->stuff, sizeof(char), datPkt->payload, fp) ;

				if (datPkt->last == YES)
				{
					lastFlag = 1 ;
					lastOffset = datPkt->offset ;
				}

				ackPkt->offset = datPkt->offset ;

				sendto (pktNo % 2 ? relayEvenSock:relayOddSock, ackPkt, sizeof (data), 0, (struct sockaddr *) ((pktNo %  2)?&relayOddAddr:&relayEvenAddr) , slen) ;
				printLog ("SERVER", "S", ackPkt, "SERVER", pktNo % 2 ? "RELAY_ODD" : "RELAY_EVEN") ;
			}
		}

		if (lastFlag && lastWindowAllRecv (recvOffsets, lastOffset))
			break ;

		FD_ZERO (&serverfd) ;
		FD_SET (serverSock, &serverfd) ;
	}

	gettimeofday (&end, NULL) ;

    time = 1000*(end.tv_sec - start.tv_sec) ;
    time += (end.tv_usec - start.tv_usec)/1000 ;

    FILE *profile ;
    profile = fopen ("prof.txt", "a") ;
    printf ("DROP = %f, Time taken = %fms\n", DROP, time) ;
    fprintf (profile, "%f %f\n", DROP, time) ;
    fclose (profile) ;

	fclose (fp) ;
	printLine () ;
	myprint ("\nFile received successfully at server\n") ;


	return 0; 
}
