#define PACKET_SIZE 100
#define TIMEOUT 2

#define myprint(x, ...) if (printFlag) printf(x, ##__VA_ARGS__ )

typedef struct _dat data ;

typedef enum _isLast
{
	YES, NO
} isLast ;

typedef enum _packetType
{
	DATA, ACK
} packetType ;

typedef enum _channelID
{
	EVEN, ODD
} channelID ;

struct _dat
{
	int payload ;
	int offset ;
	isLast last ;
	packetType pktType ;
	channelID channel ;
	char stuff [PACKET_SIZE+1] ;
} ;

char* isLastToString (isLast i) ;
char* packetTypeToString (packetType pkt) ;
char* channelIDToString (channelID cid) ;