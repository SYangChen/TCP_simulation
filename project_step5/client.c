#include "header.h"

struct sockaddr_in myaddr, remaddr;
int fd, i, j, slen=sizeof(remaddr);
char buf[BUFLEN];	/* message buffer */
int recvlen;		/* # bytes in acknowledgement message */
char *server ;	/* change this to use a different server */
unsigned short clientPort = 12345 ;
unsigned short serverPort = SERVICE_PORT ;
unsigned int sequence = 0 ;
FILE *pFile;
char video[5] = {0} ;

void UDP_socketClient() ;
void UDP_send( packet packetSend ) ;
packet UDP_receive() ;
void ThreeWayHandShake() ;
packet InitPacket() ;
void ReceiveFile() ;
void WriteData( char data[DEFAULT_MSS], int n ) ;
bool ProbLoss() { return rand()%100 == 0 ; }

int main( int argc, char *argv[] ) {
	if ( argc < 4 ) {
		printf( "Set server argument error\n" ) ;
		return 0 ;
	}
	server = malloc( 16 ) ;
	strcpy( server, argv[1] ) ;
	serverPort = atoi( argv[2] ) ;
	clientPort = atoi( argv[3] ) ;
	for ( i = 4, j = 0 ; i < argc ; ++i, ++j )
		 video[j] = argv[i][0] ;
	UDP_socketClient() ;
	ThreeWayHandShake() ;
	ReceiveFile() ;
	// UDP_send() ;
	close(fd);
}

void UDP_socketClient() {
	

	/* create a socket */

	if ((fd=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		printf("socket created\n");

	/* bind it to all local addresses and pick any port number */

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(clientPort);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return ;
	}       

	/* now define remaddr, the address to whom we want to send messages */
	/* For convenience, the host address is expressed as a numeric IP address */
	/* that we will convert to a binary format via inet_aton */

	memset((char *) &remaddr, 0, sizeof(remaddr));
	remaddr.sin_family = AF_INET;
	remaddr.sin_addr.s_addr = inet_addr(server);
	remaddr.sin_port = htons(SERVICE_PORT);
	if (inet_aton(server, &remaddr.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}
	
	return ;
}

void UDP_send( packet packetSend ) {
	/* now let's send the messages */
	// printf("Sending packet to %s port %d\n", server, packetSend.packetHeader.dstPort );
	// sprintf(buf, "This is packet %d", i);
	if (sendto(fd, (char*)&packetSend, sizeof(packetSend), 0, (struct sockaddr *)&remaddr, slen)==-1) {
		perror("sendto");
		exit(1);
	}

}

packet UDP_receive() {
	packet packetReceive ;
	/* now receive an acknowledgement from the server */
	recvlen = recvfrom(fd, (char*)&packetReceive, sizeof(packetReceive), 0, (struct sockaddr *)&remaddr, &slen);
	// clientPort = ntohs( myaddr.sin_port ) ;
	if (recvlen > 0) {
		// buf[recvlen] = 0;
		printf( "\t\treceived a packet ( seq_num = %d, ack_num = %d )\n", packetReceive.packetHeader.seqNum, packetReceive.packetHeader.ackNum );
	}
	else
		printf("uh oh - something went wrong!\n");
	return packetReceive ;
}

void ThreeWayHandShake() {
	printf( "-----Start the three-way handshake-----\n" ) ;
	packet shakePacket ;
	packet r_shakePacket ;
	shakePacket = InitPacket() ;
	shakePacket.packetHeader.flag[flagSYN] = 1 ;
	sequence = ( rand()%10000 )+1 ;
	shakePacket.packetHeader.seqNum = sequence ;
	// printf( "(SYN)" ) ;
	printf("(SYN)Sending packet to %s port %d\n", server, shakePacket.packetHeader.dstPort );
	UDP_send( shakePacket ) ;
	printf( "(SYN/ACK)" ) ;
	r_shakePacket = UDP_receive() ;
	printf("Receive packet from %s port %d\n", server, r_shakePacket.packetHeader.srcPort );
	if ( r_shakePacket.packetHeader.flag[flagSYN] && r_shakePacket.packetHeader.ackNum == shakePacket.packetHeader.seqNum+1 ) {
		shakePacket.packetHeader.flag[flagSYN] = 0 ;
		shakePacket.packetHeader.seqNum = ++sequence ;
		shakePacket.packetHeader.flag[flagACK] = 1 ;
		shakePacket.packetHeader.ackNum = r_shakePacket.packetHeader.seqNum+1 ;
		strcpy( shakePacket.data, video ) ;
		printf( "(ACK)Sending packet to %s port %d\n", server, shakePacket.packetHeader.dstPort );
		UDP_send( shakePacket ) ;
		printf( "-----Complete the three-way handshake-----\n" ) ;
	}
}

packet InitPacket() {
	packet p ;
	p.packetHeader.srcPort = clientPort ;
	p.packetHeader.dstPort = serverPort ;
	p.packetHeader.seqNum = 0 ;
	p.packetHeader.ackNum = 0 ;
	p.packetHeader.offset = ( unsigned char )80 ;
	int i ;
	for ( i = 0 ; i < 8 ; ++i )
		p.packetHeader.flag[i] = 0 ;
	p.packetHeader.rwnd = 0 ;
	p.packetHeader.checkSum = 0 ;
	p.packetHeader.urgptr = 0 ;
	return p ;
}

void ReceiveFile() {
	int v ;
	char vName[32] = { 0 } ;
	char buffer[6] = { 0 } ;
	for ( v = 0 ; v < 5 ; ++v ) {
		if ( video[v] == '0' || video[v] == '\0' ) 
			break ;
		printf( "Receive file %c from %s port %d\n", video[v], server, clientPort ) ; // ntohs( myaddr.sin_port )
		memset( vName, 0, sizeof( vName ) ) ;
		sprintf( buffer,"%ld", clientPort );
		strcat( vName, "port_" ) ;
		strcat( vName, buffer ) ;
		strcat( vName, "transmit_V" ) ;
		strncat( vName, video+v, 1 ) ;
		strcat( vName, ".mp4" ) ;
		printf( "%s\n", vName ) ;
		pFile = fopen( vName, "wb" ) ;
		if ( pFile == NULL ) {
			printf( "open write file error" ) ;
			return ;
		}
		packet receivePacket ;
		packet sendPacket = InitPacket() ;
		unsigned int cnt = 0 ;
		bool isNotLoss = 1 ;
		unsigned int clientACK = 1 ;
		while ( 1 ) {
			receivePacket = UDP_receive() ;
			if ( strcmp( receivePacket.data, "endddoffile" ) == 0 ) {
				// send fin to server
				sendPacket.packetHeader.seqNum = ++sequence ;
				sendPacket.packetHeader.ackNum = receivePacket.packetHeader.seqNum+1 ;
				sendPacket.packetHeader.flag[flagACK] = 1 ;
				sendPacket.packetHeader.flag[flagFIN] = 1 ;
				printf( "Send the FIN flag to server\n" ) ;
				UDP_send( sendPacket ) ;
				receivePacket = UDP_receive() ;
				// receive fin->ack from server
				receivePacket = UDP_receive() ;
				if ( receivePacket.packetHeader.flag[flagFIN] ) {
					sendPacket.packetHeader.seqNum = ++sequence ;
					sendPacket.packetHeader.ackNum = receivePacket.packetHeader.seqNum+1 ;
					sendPacket.packetHeader.flag[flagACK] = 1 ;
					sendPacket.packetHeader.flag[flagFIN] = 0 ;
					UDP_send( sendPacket ) ;
				}
				printf( "Close client connection\n" ) ;
				break ;
			}
			// printf( "ttt : %d\n", recvlen-sizeof( receivePacket.packetHeader ) ) ;
			if ( clientACK == receivePacket.packetHeader.seqNum ) {
				WriteData( receivePacket.data, recvlen-sizeof( receivePacket.packetHeader )-8 ) ;
				clientACK += DEFAULT_MSS ;
			}
			// if ( cnt++%2 && isNotLoss ) {
			sendPacket.packetHeader.seqNum = ++sequence ;
			sendPacket.packetHeader.ackNum = clientACK ;
			sendPacket.packetHeader.flag[flagACK] = 1 ;
			// printf( "\033[0;31mDelay ACK : %d !!!\033[0m\n", sendPacket.packetHeader.ackNum ) ;
			// if ( !ProbLoss() )
				UDP_send( sendPacket ) ;
			// else {
			// 	printf( "XXXXXXXX   Randomly Create a Packet Loss at ACK NUM : %d !!!   XXXXXXXX\n", sendPacket.packetHeader.ackNum ) ;
				// sleep ( 1 ) ;
			//}
			// }
		}
		fclose( pFile ) ;
	}
}

void WriteData( char data[DEFAULT_MSS], int n ) {
	fwrite( data, 1, n, pFile ) ;
}
