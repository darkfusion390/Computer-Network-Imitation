#include<stdlib.h> //exit(0);
#include<stdio.h> //myprint
#include<string.h> //memset
#include<unistd.h>
#include <fcntl.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include <sys/wait.h>
#include "packet.h"

int printFlag ;

void die(char *s)
{
    perror(s);
    exit(1);
}

void sendPktInWindow (int *windowPktStat, int *windowPktOffset, data **datCache, int windowStart, int noPkts, int relayEvenSock, int relayOddSock, struct sockaddr_in* relayEvenAddr, struct sockaddr_in* relayOddAddr, FILE *fp)
{
    int i , bytesRead ;
    data *datPkt ;

    for (i = 0 ; i < WINDOW_SIZE  && i + windowStart < noPkts ; i++)
    {
        if (!windowPktStat[i])
        {
            windowPktOffset[i] = (windowStart + i) * PACKET_SIZE ;
            datPkt = (data *) malloc (sizeof (data)) ;
            datPkt->offset = (windowStart + i)*PACKET_SIZE ;
            datPkt->last = (windowStart + i == noPkts-1)?YES:NO ;
            datPkt->pktType = DATA ;

            fseek (fp, (windowStart + i)*PACKET_SIZE, SEEK_SET) ;
            bytesRead = (int)fread (datPkt->stuff , sizeof(char), PACKET_SIZE, fp) ;
            datPkt->stuff[bytesRead] = '\0' ;
            datPkt->payload = bytesRead ;
            datCache[i] = datPkt ;

            sendto ((windowStart + i) % 2 ? relayOddSock : relayEvenSock, datPkt, sizeof(data), 0, (struct sockaddr *)((windowStart + i) % 2 ? relayOddAddr : relayEvenAddr), sizeof(struct sockaddr_in)) ;

            printLog ("CLIENT", "S", datPkt, "CLIENT", (windowStart + i) % 2 ? "RELAY_EVEN" : "RELAY_ODD") ;

            windowPktStat[i] = 1 ;
        }
    }

}

void sendTmoutPktInWindow (int *windowPktStat, data **datCache, int windowStart, int noPkts, int relayEvenSock, int relayOddSock, struct sockaddr_in *relayEvenAddr, struct sockaddr_in *relayOddAddr)
{
    int i ;
    for (i = 0 ; i < WINDOW_SIZE && i + windowStart < noPkts ; i++)
    {
        if (windowPktStat[i] == 1)
        {
            sendto ((windowStart + i) % 2 ? relayOddSock : relayEvenSock, datCache[i], sizeof(data), 0, (struct sockaddr *)((windowStart + i) % 2 ? relayOddAddr : relayEvenAddr), sizeof(struct sockaddr_in)) ;

            printLog ("CLIENT", "TO", datCache[i], "CLIENT", (windowStart + i) % 2 ? "RELAY_EVEN" : "RELAY_ODD") ;
            printLog ("CLIENT", "RE", datCache[i], "CLIENT", (windowStart + i) % 2 ? "RELAY_EVEN" : "RELAY_ODD") ;
        }
    }

}

int unsentInWindow (int *windowPktStat, int windowStart, int noPkts)
{
    int i ;
    for (i = 0 ; i < WINDOW_SIZE && i + windowStart < noPkts ; i++)
        if (!windowPktStat[i])
            return 1 ;

    return 0 ;
}

void initWindow (int *windowPktStat, int *windowPktOffset, data **datCache)
{
    for (int i = 0 ; i < WINDOW_SIZE ; i++)
    {
        windowPktOffset[i] = -1 ;
        windowPktStat[i] = 0 ;
        datCache[i] = NULL ;
    }
}

void shiftWindow (int *windowPktStat, int *windowPktOffset, data **datCache, int *windowStart)
{
    int i = 0, j ;
    while (i < WINDOW_SIZE && windowPktStat[i] == 2)
        i++ ;

    if (i == WINDOW_SIZE)
    {
        initWindow (windowPktStat , windowPktOffset, datCache) ;
        *windowStart += WINDOW_SIZE ;
    }
    else
    {
        for (j = i ; j < WINDOW_SIZE ; j++)
        {
            windowPktStat[j-i] = windowPktStat[j] ;
            windowPktOffset[j-i] = windowPktOffset[j] ;
            datCache[j-i] = datCache[j] ;
        }

        for (j = WINDOW_SIZE - i ; j < WINDOW_SIZE ; j++)
        {
            windowPktOffset[j] = -1 ;
            windowPktStat[j] = 0 ;
            datCache[j] = NULL ;
        }

        *windowStart += i ;
    }
}

int windowAllAck (int *windowPktStat, int windowStart, int noPkts)
{
    int i ;

    for (i = 0 ; i < WINDOW_SIZE  && i + windowStart < noPkts ; i++)
        if (windowPktStat[i] != 2)
            return 0 ;

    return 1 ;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf ("Invalid number of arguments! ./client 1 for printing enabled\n") ;
        exit (0) ;
    }

    if (!strcmp(argv[1], "1"))
        printFlag = 1 ;
    else if (!strcmp(argv[1], "0"))
        printFlag = 0 ;
    else
    {
        printf ("Print flag is incorrect. Specify as true or else\n") ;
        exit (0) ;
    }

    /* ----------------------------------------------------------------------------------------- */

	struct sockaddr_in relayEvenAddr, relayOddAddr, otherAddr ;
    int relayEvenSock, relayOddSock, i, slen = sizeof(struct sockaddr_in) ;
    int sndCount, fileSize, noPkts, bytesRead ;
    struct timeval timeout = {TIMEOUT,0} ;
    data *ackPkt, *closePkt ;
	data *datCache [WINDOW_SIZE] ;

    ackPkt = (data *) malloc (sizeof(data)) ;
    closePkt = (data *) malloc (sizeof(data)) ;

    /* ------------------------------------------------------ */

	FILE *fp = fopen ("input.txt", "r") ;
	fseek (fp, 0, SEEK_END) ;
	fileSize = ftell (fp)  ;
	noPkts = fileSize/PACKET_SIZE + ((fileSize % PACKET_SIZE)?1:0) ;
	fseek (fp, 0, SEEK_SET) ;
	myprint ("Size of file = %d\n", fileSize) ;
	myprint ("No of packets = %d\n", noPkts) ;

    /* ------------------------------------------------------ */

    int maxfd ;
	relayEvenSock = setSockAddr (&relayEvenAddr, RELAY_EVEN_PORT) ;
	relayOddSock = setSockAddr (&relayOddAddr, RELAY_ODD_PORT) ;
    fd_set relayfds ;
    FD_ZERO (&relayfds) ;
    FD_SET (relayEvenSock, &relayfds) ;
    FD_SET (relayOddSock, &relayfds) ;

    maxfd = relayEvenSock > relayOddSock ? relayEvenSock : relayOddSock ;

    /* ------------------------------------------------------ */

    int pktNo, windowStart = 0, windowSize = WINDOW_SIZE ;
    int windowPktOffset[WINDOW_SIZE] ;
    int windowPktStat[WINDOW_SIZE] ;
    initWindow (windowPktStat, windowPktOffset, datCache) ;

    int detectTmout = 1 ;
    printLine () ;
    printHeading () ;
    while (1)
    {
        if (unsentInWindow(windowPktStat, windowStart, noPkts))
            sendPktInWindow (windowPktStat, windowPktOffset, datCache, windowStart, noPkts, relayEvenSock, relayOddSock, &relayEvenAddr, &relayOddAddr, fp) ;

        detectTmout = 1 ;
        select (maxfd + 1 , &relayfds, NULL, NULL, &timeout) ;
        if (FD_ISSET (relayEvenSock, &relayfds))
        {
            detectTmout = 0 ;
            while (recvfrom (relayEvenSock, ackPkt, sizeof(data), 0, (struct sockaddr *) &relayEvenAddr, &slen) != -1)
            {
                printLog ("CLIENT", "R", ackPkt, "RELAY_EVEN", "CLIENT") ;
                pktNo = ackPkt->offset/PACKET_SIZE ;
                windowPktStat[pktNo - windowStart] = 2 ;

                if (pktNo < noPkts - WINDOW_SIZE && pktNo == windowStart)
                    shiftWindow (windowPktStat, windowPktOffset, datCache, &windowStart) ;       
            }
        }

        if (FD_ISSET (relayOddSock, &relayfds))
        {
            detectTmout = 0 ;
            while (recvfrom (relayOddSock, ackPkt, sizeof(data), 0, (struct sockaddr *) &relayOddAddr, &slen) != -1)
            {
                printLog ("CLIENT", "R", ackPkt, "RELAY_ODD", "CLIENT") ;
                pktNo = ackPkt->offset/PACKET_SIZE ;
                windowPktStat[pktNo - windowStart] = 2 ;

                if (pktNo < noPkts - WINDOW_SIZE && pktNo == windowStart)
                    shiftWindow (windowPktStat, windowPktOffset, datCache, &windowStart) ;    
            }
        }

        if (detectTmout)
            sendTmoutPktInWindow (windowPktStat, datCache, windowStart, noPkts, relayEvenSock, relayOddSock, &relayEvenAddr, &relayOddAddr) ;

        if (windowStart >= noPkts - windowSize && windowAllAck(windowPktStat, windowStart, noPkts))
            break ;

        timeout.tv_sec = TIMEOUT ;
        timeout.tv_usec = 0 ;
        FD_ZERO (&relayfds) ;
        FD_SET (relayEvenSock, &relayfds) ;
        FD_SET (relayOddSock, &relayfds) ;
    }

    printLine () ;
    myprint ("\nFile successfully uploaded!\n") ;

    closePkt->pktType = CLOSE ;
    sendto (relayEvenSock, closePkt, sizeof(data), 0, (struct sockaddr *) &relayEvenAddr, slen) ;
    sendto (relayOddSock, closePkt, sizeof(data), 0, (struct sockaddr *) &relayOddAddr, slen) ;
    	
    fclose (fp) ;
    close(relayEvenSock) ;
    close(relayOddSock) ;

    exit (0) ;
}
