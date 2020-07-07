# TCP_simulation
Computer Network Project about TCP simulation by UDP socket programming.

# Environment
Ubuntu 18.04

# Usage
#### Compile and Clean
``` bash
make all
make clean
```
#### Server process
``` bash
./server
# or 
./server "your own ip address"
# i.g. ./server 140.117.178.203
```
#### Client process
``` bash
./client "server ip" 21234 "client port" "video#1-4"
# i.g. ./client 127.0.0.1 21234 12345 1 3 4
```

## Steps
### step1
Create a server and a client.
The server and client can both transmit packets and ACKs.
The server must be able to handle multiple file requests from the client.
The client would randomly request one to four video files at one time.
### step2
The client process are increased to four clients at one time.
### step3
Channels may have losses.
The packet loss error rate is set to have a Poisson distribution with a mean of 10 for simulating the packet loss.

### step4
Delayed ACKs are added to senders and receivers.

### step5
The basic TCP congestion control.
Implement the slow start mechanism and the congestion avoidance mechanism.

### step6
The Tahoe TCP congestion control.
Implement the fast retransmit machanism. 

### step7
The Reno TCP congestion control.
Implement the Reno TCP with 3 duplicate ACKs.

### step8
The SACK option is incorporated into the standard.

### step9
Increase the number of clients to 30. 

## License
[MIT](https://choosealicense.com/licenses/mit/)