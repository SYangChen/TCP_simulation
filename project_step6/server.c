#include "header.h"

struct sockaddr_in myaddr;	/* our address */
struct sockaddr_in remaddr;	/* remote address */
struct timeval timeout ;
socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
int recvlen;			/* # bytes received */
int fd;				/* our socket */
int msgcnt = 0;			/* count # of messages we received */
unsigned char buf[BUFSIZE];	/* receive buffer */
int cwnd ;
FILE *pFile ;
int clientCnt ;
pid_t childpid ;
char *server ;
bool changeADDR = 0 ;
// for transmit //
unsigned int ssthresh = DEFAULT_THRESHOLD ;
unsigned int congestionCtrl_state ;
int i, j, v ;
int br = 0 ;
unsigned int sequence = 1 ;
unsigned int count ;
int data_len ;
int send_len ;
char video[5] = { 0 } ;
char vName[7] = { 0 } ;
unsigned short sourcePort ;
packet packetSend ;
packet receivepacket ;
unsigned int dupACKnum = 0 ;
int dupACKcount = 0 ;
bool error8193 = 0 ;
// for transmit

void UDP_socketServer( unsigned short processPort ) ;
void UDP_receive() ;
void TransmitData() ;
void SlowStart() ;
void CongestionAvoidance() ;
void FastRecovery() ;
bool ProbLoss() { return rand()%250 == 0 ; }

int main( int argc, char *argv[] ) {
	clientCnt = 0 ;
	if ( argc > 1 ) {
		server = malloc( 16 ) ;
		strcpy( server, argv[1] ) ;
		changeADDR = 1 ;
	}
	UDP_socketServer( SERVICE_PORT ) ;
	UDP_receive() ;
}

void UDP_socketServer( unsigned short processPort ) {
	
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
	myaddr.sin_port = htons(processPort);

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
		receivepacket ;
		recvlen = recvfrom(fd, (char*)&receivepacket, sizeof(receivepacket), 0, (struct sockaddr *)&remaddr, &addrlen);
		++clientCnt ;
		if (recvlen > 0) {
			// buf[recvlen] = 0;
			printf( "received a packet ( seq_num = %d, ack_num = %d )\n", receivepacket.packetHeader.seqNum, receivepacket.packetHeader.ackNum );
		}
		else
			printf("uh oh - something went wrong!\n");
		// -------------------receive then send-------------------------
		if ( receivepacket.packetHeader.flag[flagSYN] ) {
			// hand shake step 2
			if ( ( childpid = fork() ) == 0 ) {
				close( fd ) ;
				UDP_socketServer( SERVICE_PORT+clientCnt ) ;
				packet packetSend ;
				packetSend = receivepacket ;
				packetSend.packetHeader.srcPort = SERVICE_PORT+clientCnt ;
				packetSend.packetHeader.dstPort = receivepacket.packetHeader.srcPort ;
				packetSend.packetHeader.flag[flagACK] = 1 ;
				packetSend.packetHeader.ackNum = receivepacket.packetHeader.seqNum+1 ;
				packetSend.packetHeader.seqNum = 0 ;
				if (sendto(fd, (char*)&packetSend, sizeof(packetSend), 0, (struct sockaddr *)&remaddr, addrlen)==-1)
					perror("sendto");
					
				 
				timeout.tv_sec = 0 ;
				timeout.tv_usec = 200 ;

				if ( setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout) ) < 0)
					error( "setsockopt failed\n" ) ;

			}
		}
		else {
			TransmitData() ;
			if ( childpid == 0 ) {
				close( fd ) ;
				break ;
			}
		}
	}
	/* never exits */
}

void TransmitData() {
	congestionCtrl_state = SLOW_START ;
	cwnd = 1 ;
	ssthresh = DEFAULT_THRESHOLD ;
	br = 0 ;
	sequence = 1 ;
	sourcePort = receivepacket.packetHeader.srcPort ;
	
	strncpy( video, receivepacket.data, 5 ) ;
	
	
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
		printf( "===============Slow Start===============\n" ) ;
		while ( !br ) {
			count = 0 ;
			switch ( congestionCtrl_state ) {
				case 1 :
					SlowStart() ;
					break ;
				case 2 :
					CongestionAvoidance() ;
					break ;
				case 3 :
					FastRecovery() ;
					break ;
				default :
					printf( "Congestion Control FSM State Error!\n" ) ;
					break ;
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

void SlowStart() {
	count = 0 ;
	printf( "cwnd = %d, threshold = %d\n", cwnd*DEFAULT_MSS, ssthresh ) ;
	for ( i = 0 ; i < cwnd ; ++i ) {
		memset(packetSend.data, '\0', sizeof(packetSend.data));
		fseek(pFile, sequence-1, SEEK_SET);
		// control the file pointer
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
		if ( sequence == 8193 && !error8193 ) {
			error8193 = 1 ;
			printf( "Create a packet loss at byte 8193\n" ) ;
		}
		else {
			printf("\t\tSending packet to port %d ( seq_num = %d, ack_num = %d )\n", packetSend.packetHeader.dstPort, packetSend.packetHeader.seqNum, packetSend.packetHeader.ackNum );
			if (sendto(fd, (char*)&packetSend, send_len, 0, (struct sockaddr *)&remaddr, addrlen)==-1)
				perror("sendto");
		}
		sequence += DEFAULT_MSS ;
	}
	for ( i = 0 ; i < cwnd && i < count ; ++i ) {
		recvlen = recvfrom(fd, (char*)&receivepacket, sizeof(receivepacket), 0, (struct sockaddr *)&remaddr, &addrlen);
		if (recvlen > 0) {
			// buf[recvlen] = 0;
			printf( "\t\treceived a packet ( seq_num = %d, ack_num = %d )\n", receivepacket.packetHeader.seqNum, receivepacket.packetHeader.ackNum );
			sequence = receivepacket.packetHeader.ackNum ;
			// record the last sequence actually ack!!!
			if ( dupACKnum != receivepacket.packetHeader.ackNum ) {
					dupACKnum = receivepacket.packetHeader.ackNum ;
					// last ack num
					dupACKcount = 0 ;
					// cwnd += 1 ; MSS
			}
			else
				++dupACKcount ;
			if ( dupACKcount == 2 ) {
				int q ;
				for ( q = i ; q < count ; ++q )
					recvlen = recvfrom(fd, (char*)&receivepacket, sizeof(receivepacket), 0, (struct sockaddr *)&remaddr, &addrlen);
				printf( "Receive Three Duplicate ACK!!!\n" ) ;
				printf( "------ FAST RETRANSMIT ------\n" ) ;
				ssthresh = cwnd*DEFAULT_MSS/2 ;
				cwnd = 1 ;	// 1 MSS
				congestionCtrl_state = SLOW_START ;
				dupACKcount = 0 ;
				sequence = dupACKnum ;
				dupACKnum = 0 ;
				printf( "===============Slow Start===============\n" ) ;
				// reset and resend the loss packet
				return ;
			}
		}
		else {
			printf( "\033[0;31mTime OUT!!!\033[0m\n" ) ;
			ssthresh = cwnd*DEFAULT_MSS/2 ;
			cwnd = 1 ;	// 1 MSS
			congestionCtrl_state = SLOW_START ;
			printf( "===============Slow Start===============\n" ) ;
			// reset and resend the loss packet
			return ;
			// dupACKcount = 0 ;
			// printf("uh oh - something went wrong!\n");
		}
	}
	if ( br ) return ;
	if ( count == i )
		cwnd += cwnd ;
	if ( cwnd*DEFAULT_MSS >= ssthresh ) {
		congestionCtrl_state = CONGESTION_AVOIDANCE ;
		printf( "==========Congestion Avoidance==========\n" ) ;
	}
}

void CongestionAvoidance() {
	count = 0 ;
	printf( "cwnd = %d, threshold = %d\n", cwnd*DEFAULT_MSS, ssthresh ) ;
	for ( i = 0 ; i < cwnd ; ++i ) {
		memset(packetSend.data, '\0', sizeof(packetSend.data));
		fseek(pFile, sequence-1, SEEK_SET);
		// control the file pointer
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
		if ( !ProbLoss() ) {
			printf("\t\tSending packet to port %d ( seq_num = %d, ack_num = %d )\n", packetSend.packetHeader.dstPort, packetSend.packetHeader.seqNum, packetSend.packetHeader.ackNum );
			if (sendto(fd, (char*)&packetSend, send_len, 0, (struct sockaddr *)&remaddr, addrlen)==-1)
				perror("sendto");
		}
		else
			printf( "XXXXXXXX   Randomly Create a Packet Loss at Seq NUM : %d !!!   XXXXXXXX\n", packetSend.packetHeader.seqNum ) ;
		sequence += DEFAULT_MSS ;
	} // send/receive
	sequence = sequence-cwnd*DEFAULT_MSS ;
	for ( i = 0 ; i < cwnd && i < count ; ++i ) {
		recvlen = recvfrom(fd, (char*)&receivepacket, sizeof(receivepacket), 0, (struct sockaddr *)&remaddr, &addrlen);
		if (recvlen > 0) {
			// buf[recvlen] = 0;
			// if ( receivepacket.packetHeader.ackNum-sequence <= 1024 ) {
			printf( "\t\treceived a packet ( seq_num = %d, ack_num = %d )\n", receivepacket.packetHeader.seqNum, receivepacket.packetHeader.ackNum );
			sequence = receivepacket.packetHeader.ackNum ;
			// }
			// record the last sequence actually ack!!!
			if ( dupACKnum != receivepacket.packetHeader.ackNum ) {
					dupACKnum = receivepacket.packetHeader.ackNum ;
					// last ack num
					dupACKcount = 0 ;
					// cwnd += 1 ; MSS
			}
			else
				++dupACKcount ;
			if ( dupACKcount == 2 ) {
				int q ;
				for ( q = i ; q < count ; ++q )
					recvlen = recvfrom(fd, (char*)&receivepacket, sizeof(receivepacket), 0, (struct sockaddr *)&remaddr, &addrlen);
				printf( "Receive Three Duplicate ACK!!!\n" ) ;
				printf( "------ FAST RETRANSMIT ------\n" ) ;
				ssthresh = cwnd*DEFAULT_MSS/2 ;
				cwnd = 1 ;	// 1 MSS
				congestionCtrl_state = SLOW_START ;
				dupACKcount = 0 ;
				sequence = dupACKnum ;
				dupACKnum = 0 ;
				printf( "===============Slow Start===============\n" ) ;
				// reset and resend the loss packet
				return ;
			}
		}
		else {
			printf( "\033[0;31mTime OUT!!!\033[0m\n" ) ;
			ssthresh = cwnd*DEFAULT_MSS/2 ;
			cwnd = 1 ;	// 1 MSS
			congestionCtrl_state = SLOW_START ;
			printf( "===============Slow Start===============\n" ) ;
			return ;
			// dupACKcount = 0 ;
			// printf("uh oh - something went wrong!\n");
		}
	}
	if ( br ) return ;
	if ( count == i )
		cwnd += 1 ;
}

void FastRecovery() {
	;
}
