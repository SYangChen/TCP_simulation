#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>

#define DEFAULT_RTT	15				// ms
#define DEFAULT_THRESHOLD 65536		// bytes
#define DEFAULT_MSS  1024			// bytes
#define DEFAULT_BUFFER_SIZE 524288	// bytes
#define SERVICE_PORT 21234			// port
#define BUFSIZE 2048
#define BUFLEN 2048
#define MSGS 1	/* number of messages to send */
#define flagURG 2
#define flagACK 3   
#define flagPSH 4
#define flagRST 5
#define flagSYN 6
#define flagFIN 7
#define SLOW_START 1
#define CONGESTION_AVOIDANCE 2
#define FAST_RECOVERY 3

typedef struct tcpHeader {

	unsigned short srcPort ;
	unsigned short dstPort ;
	unsigned int seqNum ;
	unsigned int ackNum ;
	unsigned char offset ;	// headlen+notused
	bool flag[8] ;
	/*
	bool flagURG ;	2
	bool flagACK ;	3   
	bool flagPSH ;	4
	bool flagRST ;	5
	bool flagSYN ;	6
	bool flagFIN ;	7
	*/
	unsigned short rwnd ;
	unsigned short checkSum ;
	unsigned short urgptr ;
	
}tcpHeader;

typedef struct packet {

	tcpHeader packetHeader ;
	unsigned char data[DEFAULT_MSS+5] ;

}packet;
