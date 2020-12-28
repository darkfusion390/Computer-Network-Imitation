#include <stdio.h>
#include <stdlib.h>
#include "packet.h"

#define myprint(x, ...) if (printFlag) printf(x, ##__VA_ARGS__ )

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
		default :
			return "packetType_ERROR" ;	
	}
}

char* channelIDToString (channelID cid)
{
	switch (cid)
	{
		case EVEN :
			return "EVEN" ;
		case ODD :
			return "ODD" ;
		default :
			return "channelID_ERROR" ;
	}
}
