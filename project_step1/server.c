#include "header.h"

struct sockaddr_in myaddr;	/* our address */
struct sockaddr_in remaddr;	/* remote address */
socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
int recvlen;			/* # bytes received */
int fd;				/* our socket */
int msgcnt = 0;			/* count # of messages we received */
unsigned char buf[BUFSIZE];	/* receive buffer */
int cwnd ;
FILE *pFile ;
char *server ;
bool changeADDR = 0 ;

void UDP_socketServer() ;
void UDP_receive() ;
void TransmitData( packet receivepacket ) ;

int main( int argc, char *argv[] ) {
	if ( argc > 1 ) {
		server = malloc( 16 ) ;
		strcpy( server, argv[1] ) ;
		changeADDR = 1 ;
	}
	UDP_socketServer() ;
	UDP_receive() ;
}

void UDP_socketServer() {
	
	/* create a UDP socket */

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return ;
	}

	/* bind the socket to any valid IP address and a specific port */

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	if ( changeADDR )
		myaddr.sin_addr.s_addr = inet_addr(server) ;
	else
		myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(SERVICE_PORT);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return ;
	}
}

void UDP_receive() {
	/* now loop, receiving data and printing what we received */
	printf("Server start : waiting on port %d\n", SERVICE_PORT);
	for (;;) {
		// printf("waiting on port %d\n", SERVICE_PORT);
		packet receivepacket ;
		recvlen = recvfrom(fd, (char*)&receivepacket, sizeof(receivepacket), 0, (struct sockaddr *)&remaddr, &addrlen);
		if (recvlen > 0) {
			// buf[recvlen] = 0;
			printf( "received a packet ( seq_num = %d, ack_num = %d )\n", receivepacket.packetHeader.seqNum, receivepacket.packetHeader.ackNum );
		}
		else
			printf("uh oh - something went wrong!\n");
		// -------------------receive then send-------------------------
		packet packetSend ;
		if ( receivepacket.packetHeader.flag[flagSYN] ) {
			// hand shake step 2
			packetSend = receivepacket ;
			packetSend.packetHeader.srcPort = SERVICE_PORT ;
			packetSend.packetHeader.dstPort = receivepacket.packetHeader.srcPort ;
			packetSend.packetHeader.flag[flagACK] = 1 ;
			packetSend.packetHeader.ackNum = receivepacket.packetHeader.seqNum+1 ;
			packetSend.packetHeader.seqNum = 0 ;
			if (sendto(fd, (char*)&packetSend, sizeof(packetSend), 0, (struct sockaddr *)&remaddr, addrlen)==-1)
				perror("sendto");
		}
		else {
			TransmitData( receivepacket ) ;
		}
	}
	/* never exits */
}

void TransmitData( packet receivepacket ) {
	cwnd = 1 ;
	int i, j, v ;
	int br = 0 ;
	unsigned int sequence = 1 ;
	unsigned int count ;
	int data_len ;
	int send_len ;
	char video[5] = { 0 } ;
	char vName[7] = { 0 } ;
	unsigned short sourcePort = receivepacket.packetHeader.srcPort ;
	strncpy( video, receivepacket.data, 5 ) ;
	
	packet packetSend ;
	
	for ( v = 0 ; v < 5 ; ++v ) {
		if ( video[v] == '0' || video[v] == '\0' )
			break ;
		memset( vName, 0, sizeof( vName ) ) ;
		strcat( vName, "V" ) ;
		strncat( vName, video+v, 1 ) ;
		strcat( vName, ".mp4" ) ;
		pFile = fopen( vName, "r" ) ;
		if ( pFile == NULL ) {
			printf( "open read file error" ) ;
			return ;
		}
		packetSend.packetHeader.srcPort = SERVICE_PORT ;
		packetSend.packetHeader.dstPort = sourcePort ;
		packetSend.packetHeader.flag[flagACK] = 0 ;
		packetSend.packetHeader.ackNum = 0 ;
		packetSend.packetHeader.seqNum = sequence ;
		br = 0 ;
		while ( !br ) {
			count = 0 ;
			for ( i = 0 ; i < cwnd ; ++i ) {
				memset(packetSend.data, '\0', sizeof(packetSend.data));
				data_len = fread(packetSend.data, sizeof(char), DEFAULT_MSS, pFile);
				// printf( "ttttt  %s\n", packetSend.data ) ;
				send_len = sizeof( packetSend )+data_len-1024 ;
				// printf( "ttt : %d\n", send_len ) ;
				if ( data_len <= 0 ) {
					br = 1 ;
					break ;
				}
				packetSend.packetHeader.seqNum = sequence ;
				++count ;
				if (sendto(fd, (char*)&packetSend, send_len, 0, (struct sockaddr *)&remaddr, addrlen)==-1)
					perror("sendto");
				sequence += DEFAULT_MSS ;
			}
			for ( i = 0 ; i < cwnd && i < count ; ++i ) {
				recvlen = recvfrom(fd, (char*)&receivepacket, sizeof(receivepacket), 0, (struct sockaddr *)&remaddr, &addrlen);
				if (recvlen > 0) {
					// buf[recvlen] = 0;
					printf( "\t\treceived a packet ( seq_num = %d, ack_num = %d )\n", receivepacket.packetHeader.seqNum, receivepacket.packetHeader.ackNum );
				}
				else
					printf("uh oh - something went wrong!\n");
				// cwnd += cwnd ;
			}
			if ( br ) {
				memset(packetSend.data, 0, sizeof(packetSend.data));
				strcpy ( packetSend.data, "endddoffile" ) ;
				packetSend.packetHeader.seqNum = sequence ;
				++count ;
				if (sendto(fd, (char*)&packetSend, sizeof(packetSend), 0, (struct sockaddr *)&remaddr, addrlen)==-1)
					perror("sendto");
				// sequence += DEFAULT_MSS ;
				
				recvlen = recvfrom(fd, (char*)&receivepacket, sizeof(receivepacket), 0, (struct sockaddr *)&remaddr, &addrlen);
				printf( "\t\treceived a packet ( seq_num = %d, ack_num = %d )\n", receivepacket.packetHeader.seqNum, receivepacket.packetHeader.ackNum );
				if ( receivepacket.packetHeader.flag[flagFIN] ) {
					printf( "Receive client FIN flag of the connection port : %d\n", receivepacket.packetHeader.srcPort ) ; 
					// receive fin flag
					packetSend.packetHeader.seqNum = ++sequence ;
					packetSend.packetHeader.ackNum = receivepacket.packetHeader.seqNum ;
					packetSend.packetHeader.flag[flagACK] = 1 ;
					packetSend.packetHeader.flag[flagFIN] = 0 ;
					// send fin->ack
					if (sendto(fd, (char*)&packetSend, sizeof(packetSend), 0, (struct sockaddr *)&remaddr, addrlen)==-1)
						perror("sendto");
					// sequence += DEFAULT_MSS ;
					sleep( 1 ) ;
					// wait then send fin flag to client
					packetSend.packetHeader.flag[flagACK] = 0 ;
					packetSend.packetHeader.flag[flagFIN] = 1 ;
					packetSend.packetHeader.seqNum = ++sequence ;
					packetSend.packetHeader.ackNum = 0 ;
					if (sendto(fd, (char*)&packetSend, sizeof(packetSend), 0, (struct sockaddr *)&remaddr, addrlen)==-1)
						perror("sendto");
					// sequence += DEFAULT_MSS ;
					recvlen = recvfrom(fd, (char*)&receivepacket, sizeof(receivepacket), 0, (struct sockaddr *)&remaddr, &addrlen);
					printf( "\t\treceived a packet ( seq_num = %d, ack_num = %d )\n", receivepacket.packetHeader.seqNum, receivepacket.packetHeader.ackNum );
					// receive ack flag
					printf( "Close server socket of the connection port : %d\n", receivepacket.packetHeader.srcPort ) ; 
				}
			}
		}
		fclose( pFile ) ;
	}
}
