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
int clientCnt ;
pid_t childpid ;
char *server ;
bool changeADDR = 0 ;
unsigned long int N = 30;
unsigned long int f[30];
 
void Factorial()
{
    f[0] = 0;
    f[1] = 1;
    for (int i=2; i<=N; ++i)
        f[i] = f[i-1] * i;
}

bool PoissonLoss( int k ) {
	double prob ;
	
	// srand( time( NULL ) ) ;
	// for ( k = 1 ; k < 20 ; ++k ) {
	prob = (exp(-LAMBDA) * pow( LAMBDA, k )) / f[k] ;
	// printf( "%lf  ", prob ) ;
	// printf( "%d\n", rand()%( 130-(int)(prob*1000) ) ) ;
	return rand()%( 1300-(int)(prob*10000) ) == 0 ;
	// }
}

void UDP_socketServer( unsigned short processPort ) ;
void UDP_receive() ;
void TransmitData( packet receivepacket ) ;
// bool PoissonLoss() { return !( rand()%100 ) ; }
unsigned short CaculateCheckSum( packet p ) ;

int main( int argc, char *argv[] ) {
	srand( time( NULL ) ) ;
	Factorial() ;
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
		packet receivepacket ;
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
			}
		}
		else {
			TransmitData( receivepacket ) ;
			if ( childpid == 0 ) {
				close( fd ) ;
				break ;
			}
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
				if ( PoissonLoss( sequence/200000 ) )
					packetSend.packetHeader.checkSum = 87 ;
				else
					packetSend.packetHeader.checkSum = CaculateCheckSum( packetSend ) ;
				printf( "***SendCheckSum %d ****************\n", packetSend.packetHeader.checkSum ) ;
				if ( sendto(fd, (char*)&packetSend, send_len, 0, (struct sockaddr *)&remaddr, addrlen)==-1)
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
			}
		}
		fclose( pFile ) ;
	}
}

unsigned short CaculateCheckSum( packet p ) {
	unsigned short sum = 0 ;
	unsigned short temp = 0 ;
	sum += p.packetHeader.srcPort ;
	sum += p.packetHeader.dstPort ;
	if ( 65535-p.packetHeader.srcPort < p.packetHeader.dstPort ) ++sum ;
	
	temp = ( p.packetHeader.seqNum >> 16 ) ;
	sum += temp ;
	
	if ( 65535-sum < temp ) ++sum ;
	temp = p.packetHeader.seqNum ;
	sum += temp ;
	if ( 65535-sum < temp ) ++sum ;
	
	temp = ( p.packetHeader.ackNum >> 16 ) ;
	sum += temp ;
	if ( 65535-sum < temp ) ++sum ;
	temp = p.packetHeader.ackNum ;
	sum += temp ;
	if ( 65535-sum < temp ) ++sum ;
	
	uint16_t *data_pointer = (uint16_t *) p.data ;
	int bytes = sizeof( p.data ) ;
	while(bytes > 1){
        temp = (unsigned short)*data_pointer++;
        //If it overflows to the MSBs add it straight away
        sum += temp ;
        if ( 65535-sum < temp ) ++sum ;
        bytes -= 2; //Consumed 2 bytes
    }
    return ~sum ;
}
