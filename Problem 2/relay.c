//Example code: A simple server side code, which echos back the received message. 
//Handle multiple socket connections with select and fd_set on Linux 
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
#include <sys/wait.h>
#include <time.h> 
#include <fcntl.h>
#include "packet.h"
	
#define TRUE 1 
#define FALSE 0 

int printFlag ;

double getDelay ()
{
	return 2 * (double)rand()/(double)(RAND_MAX) ;
}

double getRand ()
{
	return (double)rand()/(double)(RAND_MAX) ;
}

int main(int argc , char *argv[]) 
{ 
	if (argc != 3)
	{
		printf ("Invalid number of arguments. ./relay 0.45 1 for drop probability 0.45 and printing enabled\n") ;
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

 
	/* ------------------------------------------------------------------------------------------------- */
	struct sockaddr_in serverAddr ;
	int serverSock ;
	serverSock = setSockAddr (&serverAddr, SERVER_PORT) ;

	struct sockaddr_in relayEvenAddr, relayOddAddr, clientAddr, otherAddr ; 
	int relayEvenSock, relayOddSock, new_socket, clientSock, slen = sizeof(struct sockaddr_in ) ;
	int i, max_sd, sd, disconnect = 0, activity, flags ;
	double delay ;

	data *pkt = (data *) malloc (sizeof (data)) ;
	data *datPkt, *ackPkt ;

	srand (time(0)) ;

	if (fork())
	{
		int evenCloseFlag = 0 ;
		relayEvenSock = setSockAddrBind (&relayEvenAddr, RELAY_EVEN_PORT) ;

		fd_set evenfd ;
		FD_ZERO (&evenfd) ;
		FD_SET (relayEvenSock, &evenfd) ;

		myprint("RELAY_EVEN : Waiting for connection\n"); 
		sleep (0.0001) ;
		while (TRUE)
		{
			select (relayEvenSock + 1 , &evenfd, NULL, NULL, NULL) ;
			if (FD_ISSET (relayEvenSock, &evenfd))
			{
				while (recvfrom(relayEvenSock , pkt, sizeof(data), 0, (struct sockaddr *) &otherAddr, &slen) != -1)
				{
					if (pkt->pktType == CLOSE)
					{
						evenCloseFlag = 1 ;
						break ;
					}
					else if (pkt->pktType == DATA)
					{
						datPkt = pkt ;
						printLog ("RELAY_EVEN", "R", datPkt, "CLIENT", "RELAY_EVEN") ;
						clientAddr = otherAddr ;
			
						if (getRand () > DROP)
						{
							delay = getDelay () ;
							if (!fork())
							{
								sleep (delay/1000) ;

								sendto (serverSock, datPkt, sizeof(data), 0, (struct sockaddr *) &serverAddr, slen) ;
								printLog ("RELAY_EVEN", "S", datPkt, "RELAY_EVEN", "SERVER") ;

								exit (0) ;
							}
						}
						else
							printLog ("RELAY_EVEN", "D", datPkt, "CLIENT", "RELAY_EVEN") ;
					}
					else
					{
						ackPkt = pkt ;
						printLog ("RELAY_EVEN", "R", ackPkt, "SERVER", "RELAY_EVEN") ;
						
						sendto (relayEvenSock, ackPkt, sizeof(data), 0, (struct sockaddr *)&clientAddr, slen) ;
						printLog ("RELAY_EVEN", "S", ackPkt, "RELAY_EVEN", "CLIENT") ;
					}
				}
			}

			FD_ZERO (&evenfd) ;
			FD_SET (relayEvenSock, &evenfd) ;

			if (evenCloseFlag)
				break ;
		}

		wait (NULL) ;
		printLine () ;
		myprint ("\nEven relay closed\n") ;
		myprint ("Odd relay closed\n") ;
		close (relayEvenSock) ;
	}
	else
	{
		int oddCloseFlag = 0 ;
		relayOddSock = setSockAddrBind (&relayOddAddr, RELAY_ODD_PORT) ;

		fd_set oddfd ;
		FD_ZERO (&oddfd) ;
		FD_SET (relayOddSock, &oddfd) ;
		FD_SET (serverSock, &oddfd) ;
		
		myprint("RELAY_ODD : Waiting for connection\n") ; 
		printLine () ;
		printHeading () ;
		
		while (TRUE)
		{
			select (relayOddSock + 1, &oddfd, NULL, NULL, NULL) ;
			if (FD_ISSET (relayOddSock, &oddfd))
			{
				while (recvfrom(relayOddSock , pkt, sizeof(data), 0, (struct sockaddr *) &otherAddr, &slen) != -1)
				{
					if (pkt->pktType == CLOSE)
					{
						oddCloseFlag = 1 ;
						break ;
					}
					else if (pkt->pktType == DATA)
					{
						datPkt = pkt ;
						printLog ("RELAY_ODD", "R", datPkt, "CLIENT", "RELAY_ODD") ;

						clientAddr = otherAddr ;	
						if (getRand () > DROP)
						{
							delay = getDelay () ;
							if (!fork())
							{
								sleep (delay/1000) ;

								sendto (serverSock, datPkt, sizeof(data), 0, (struct sockaddr *) &serverAddr, slen) ;
								printLog ("RELAY_ODD", "S", datPkt, "RELAY_ODD", "SERVER") ;

								exit (0) ;
							}
						}
						else
							printLog ("RELAY_ODD", "D", datPkt, "CLIENT", "RELAY_ODD") ;
					}
					else
					{
						ackPkt = pkt ;
						printLog ("RELAY_ODD", "R", ackPkt, "SERVER", "RELAY_ODD") ;

						sendto (relayOddSock, ackPkt, sizeof(data), 0, (struct sockaddr *)&clientAddr, slen) ;
						printLog ("RELAY_ODD", "S", ackPkt, "RELAY_ODD", "SERVER") ;
					}
				}
			}

			if (oddCloseFlag)
				break ;
		
			FD_ZERO (&oddfd) ;
			FD_SET (relayOddSock, &oddfd) ;
		}

		close (relayOddSock) ;
	}
}