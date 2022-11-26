//
// @author: Leo Framba, Alex
//
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include </Users/lframba/CLionProjects/CS336/Final-Project/Server/Config_Parser.c>

#define config "/Users/lframba/CLionProjects/CS336/Final-Project/Server/config.txt"
#define DATA_THRESHOLD .95

/**
 * Function for throwing errors
 * @param msg - The Error message the user sees
 */
void e(char *msg) {
    perror(msg);
    exit(1);
}

void recvFile(int comSock) {

    //Open a file to write data to
    FILE *file = fopen(config, "w");
    if (!file) {
        e("Error opening config file for write");
    }

    //Data buffer
    char buffer[3000];
    int bufferSize = sizeof(buffer);

    //Get config file from client
    long charsReceived;
    while (1) {
        charsReceived = recv(comSock, buffer, bufferSize, 0);
        if (charsReceived <= 0) {
            break;
        }
        fprintf(file, "%s", buffer);
        bzero(buffer, bufferSize);
    }

    printf("Received Config file.\n");
    //Close file
    fclose(file);
}

void send_file(int sock, FILE *file) {

    //Intialize buffer
    char buffer[3000] = {0};
    int bufferSize = sizeof buffer;

    //While there is data in the file get it
    while (fgets(buffer, bufferSize, file) != NULL) {
        //Send data over socket
        if (send(sock, buffer, bufferSize, 0) < 0) {
            e("Error Sending file.");
        }
        //Clear buffer for new data
        bzero(buffer, bufferSize);
    }
}

int openTcpConnection(int port) {
    //Open a socket that listens for client connections
    int listenSock = socket(PF_INET, SOCK_STREAM, PF_UNSPEC);
    if (listenSock < 0) {
        e("Error opening TCP socket\n");
    }
    printf("Created TCP Socket.\n");

    //Define Socket address struct + size variable
    struct sockaddr_in addr;
    socklen_t addrSize = sizeof addr;

    //Set address Settings
    bzero(&addr, addrSize);
    addr.sin_family = PF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    //Bind port + socket
    if (bind(listenSock, (const struct sockaddr *) &addr, addrSize) < 0) {
        e("Error Binding TCP socket\n");
    }
    printf("Bound TCP socket.\n");

    //Listen for the client
    if (listen(listenSock, 1) < 0) {
        e("Error listening for TCP client");
    }
    printf("Listening on port: %i...\n", port);

    //Accept client connection
    int comSock = accept(listenSock, (struct sockaddr *) &addr, &addrSize);
    if (comSock < 0) {
        e("Error accepting client connection.\n");
    }
    printf("Accepted client connection.\n");

    //Close Listening Socket once we accept connection
    close(listenSock);
    printf("Closed Listening Socket\n");

    return comSock;
}


void getConfig(int port) {

    //Open a tcp connection and get a com sock
    int comSock = openTcpConnection(port);

    //Recv file w/ comsocket
    recvFile(comSock);

    //Close socket after receiving file
    close(comSock);
    printf("Closed TCP connection socket.\n");
}

struct packetProbe {
    long time;
    int lastPacketRecv;
    int packetsRecv;
};

struct timespec getDelta(struct timespec start, struct timespec end) {
    struct timespec temp;

    if ((end.tv_nsec - start.tv_nsec) < 0) {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}


int getUdpPacketTrain(long packCount, int payloadSize, int sock, struct sockaddr_in addr, socklen_t addrSize, struct packetProbe *probe) {

    printf("Receiving Packet Train...\n");

    int status = 1;

    //Negative time if no packets are recv
    probe->time = -1;
    //Set packetsRecv to true unless we reach the end
    probe->packetsRecv = 0;
    //Buffer for data
    unsigned char buffer[payloadSize];

    //Time markers
    struct timespec start, end;

    //Set to -1 for 0 packets received
    int currPacketId = -1;

    //Receive first packet to start timer
    if (recvfrom(sock, buffer, payloadSize, 0, (struct sockaddr *) &addr, &addrSize) < 0) {
        printf("Timeout @%i\n", currPacketId);
        return -1;
    }
    currPacketId = buffer[1] + (buffer[0] << 8);

    //Log: Print for debug
    //printf("i: %i, Id:%u, Pay: %s.\n",0,  buffer[1] + (buffer[0]<<8), &buffer[3]);

    //Start Time
    clock_gettime(CLOCK_REALTIME, &start);

    //Calc threshold packet #
    int threshold  = packCount * DATA_THRESHOLD;

    //Receive Remaining Packets
    for (int i = 1; i < packCount; i++) {

        //Timeout breaks packetProbe
        if (recvfrom(sock, buffer, payloadSize, 0, (struct sockaddr *) &addr, &addrSize) < 0) {
            printf("Timeout @%i\n", currPacketId);
            status = -1;
            break;
        }
        probe->packetsRecv++;

        //Keep track of id of the last packet received.
        currPacketId = buffer[1] + (buffer[0] << 8);

        /*
         * Set current end time. We only care about the final packets that arrive.
         * Packets that arrive before this threshold are omitted from the time check because
         * they would show quicker numbers than expected
         */
        if (i > threshold) {
            clock_gettime(CLOCK_REALTIME, &end);
        }

        //Log: Print for debug
        //printf("i: %i, Id:%u, Pay: %s.\n",i,  buffer[1] + (buffer[0]<<8), &buffer[3]);
        //Clear buffer
        bzero(buffer, payloadSize);
    }

    //Log: Packet train complete
    printf("Packet Train Complete\n");

    probe->lastPacketRecv = currPacketId;
    //Calc Time - convert to msec
    probe->time = getDelta(start, end).tv_nsec / 1000000;
    return status;
}


int main(int argc, char **argv){

    //Check arg count for 2. 1) program name 2) config file path
    if(argc < 2){
        puts("Error: Too Few Args");
        exit(0);
    }else if(argc > 2) {
        puts("Error: Too many Args");
        exit(0);
    }

    printf("Arg Count: %i\nConfig file: %s\n", argc, argv[1]);
    //IF we have correct args. Parse config file which should be arg 2

    //Port to listen on
    int port = 8080;

    /*************************------Pre Probing Phase------*************************/
    printf("\n********** Pre Probing Phase **********\n");
    //Open a TCP connection, Listen + Accept a client, return communication socket w/ client
    getConfig(port);

    struct config_settings *settings = parse_config(config);
    puts("Finished Config parse\n");

    /*************************------Probing Phase------*************************/
    printf("\n********** Probing Phase **********\n");

    //Create UDP socket to accept packet train from client
    int udpSock = socket(PF_INET, SOCK_DGRAM, PF_UNSPEC);
    if (udpSock < 0) {
        e("Failed to open UDP socket.");
    }
    //Log: Open udp socket
    printf("Opened UDP socket.\n");

    /*
     * We set the packetsRecv for the udp sockets because UDP does not guarantee that we will
     * receive all packets from the train.
     * The packetsRecv is set to the agreed upon intersection time -2. This allows the server to
     * get as many packets as possible while still having time to catch the next packet train.
     */
    struct timeval timeout = {settings->InterTime - 3, 0};
    setsockopt(udpSock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval));

    //Define addr
    struct sockaddr_in addr;
    socklen_t addrSize = sizeof addr;
    bzero(&addr, addrSize);
    //addr settings
    addr.sin_family = PF_INET;
    addr.sin_port = htons(settings->destPortUdp);
    addr.sin_addr.s_addr = inet_addr(settings->serverIp);

    //Bind
    if (bind(udpSock, (struct sockaddr *) &addr, sizeof addr) < 0) {
        e("Failed to bind UCP");
    }
    printf("Bound UDP socket.\n");

    //Receive low entropy packet
    struct packetProbe *lowEntropy = malloc(sizeof (struct packetProbe));
    int to = getUdpPacketTrain(settings->UdpPackCount, settings->UdpPayLoadSize, udpSock, addr,
                               addrSize, lowEntropy);

    //If there's packetsRecv in low entropy skip sleep to try and catch high entropy data
    if (to < 0) {
        sleep(1);
    }else{
        sleep(settings->InterTime - 2);
    }

    //Get high entropy data
    struct packetProbe *highEntropy = malloc(sizeof (struct packetProbe));
    to = getUdpPacketTrain(settings->UdpPackCount, settings->UdpPayLoadSize, udpSock, addr,
                           addrSize, highEntropy);
    //Close socket after receiving data
    close(udpSock);

    //Log: Print findings from probe
    printf("LowEntropy took %ld, LPR: %i, PacksRecv: %i\n", lowEntropy->time, lowEntropy->lastPacketRecv, lowEntropy->packetsRecv);
    printf("HighEntropy took %ld, LPR: %i, , PacksRecv: %i\n", highEntropy->time, highEntropy->lastPacketRecv, highEntropy->packetsRecv);
    printf("Delta %li msecs:\n", (highEntropy->time - lowEntropy->time));


    /*************************------POST Probing Phase------*************************/
    printf("\n********** Post Probing Phase **********\n");
    //Initiate a new tcp socket
    int comSock = openTcpConnection(port);

    /*
     * Results are stored in a fixed array
     * [0] = is the time taken for low entropy
     * [1] = is the time taken for high entropy
     * [2] = is an error flag. 0 = Sufficient data was captured, 1 = insufficient data due to packetsRecv/not enough packets
     */
    long results[3];
    results[0] = lowEntropy->time;
    results[1] = highEntropy->time;
    results[2] = highEntropy->time < 0 || lowEntropy->time < 0;

    if (send(comSock, results, sizeof results, 0) < sizeof results) {
        e("Failed to send results to client");
    }
    close(comSock);

    //Parse config setting file into a settings struct
    //struct config_settings *settings = parse_config(argv[1]);
    //initServer(8080);
}
