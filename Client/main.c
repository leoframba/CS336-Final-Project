# include <stdio.h>
# include <stdlib.h>
# include <strings.h>
#include <netinet/in.h>
#include "Config_Parser.c"
#include "/Users/lframba/CLionProjects/CS336/CS336_FianlProject_Client/JSON/cJSON.h"
#include <unistd.h>

/**
 * Error printing method
 *
 * @param msg Error msg to print
 */
void e(char *msg){
    perror(msg);
    exit(1);
}

/**
 * Sends a file over a socket
 *
 * @param sock Socket to send file over
 * @param file File to send
 */
void send_file(int sock, FILE *file){

    //Intialize buffer
    char buffer[3000] = {0};
    int bufferSize = sizeof buffer;

    //While there is data in the file get it
    while(fgets(buffer, bufferSize, file) != NULL){
        //Send data over socket
        if(send(sock, buffer, bufferSize, 0 ) < 0){
            e("Error Sending file.");
        }
        //Clear buffer for new data
        bzero(buffer, bufferSize);
    }
}

int initTcpSock(int port, unsigned int serverIp ){

    //Create socket for TCP connection
    int sock = socket(PF_INET, SOCK_STREAM, PF_UNSPEC);
    if(sock < 0){
        e("Error creating socket\n");
    }
    //Log: TCP socket creation
    printf("Created Socket with status: %i\n", sock);

    //Create addr
    struct sockaddr_in addr;
    int addrSize = sizeof (addr);
    bzero(&addr, addrSize);


    //Set address Settings
    addr.sin_family = PF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = serverIp;


    //Attempt TCP connect to server
    int status = connect(sock, (const struct sockaddr *) &addr, addrSize);
    if(status < 0){
        e("Error connecting to server\n");
    }
    printf("Connected to server @%d::%i\n", serverIp, port);

    //Return connected socket
    return sock;
}

int sendUdpPacketTrain(
        long packCount, int payloadSize, unsigned char * payload, int udpSock, struct sockaddr_in udpAddr){

    //Log: Sending UDP packet Train
    printf("Sending UDP packet train...\n");

    long status;

    //Confirm payload has at least 1 byte of payload
    //First two bytes are reserved for pack id
    if(payloadSize <= 2) {
        return -1;
    }

    int udpAddrSize = sizeof udpAddr;

    //Set the first two bytes to zero to use for pack id
    payload[0] = 0;
    payload[1] = 0;

    //Wait
    sleep(1);

    //Send packs
    for(int i = 0; i < packCount; i++){

        //Log: Print sent packet
        //printf("i: %i, Id:%u, Pay: %s.\n",i,  payload[1] + (payload[0]<<8), &payload[3]);

        //Send packet
        status = sendto(udpSock, payload, payloadSize, 0, (const struct sockaddr *) &udpAddr, udpAddrSize);
        if(status <= 0) {
            return -1;
        }

        //Increment payload id
        if(payload[1] == 255){
            payload[0]++;
            payload[1] = 0;
        } else {
            payload[1]++;
        }

        //Sleep for cleaner data transfer
        usleep(200);
    }

    //Log: Finished Sending UDP packet Train
    printf("Finished sending UDP packet train.\n");

    return 1;
}

/**
 * Client app
 *
 * @param argc # of Arguments
 * @param argv Values of arguments
 * @return 1 if fail, 0 if success
 */
int main(int argc, char **argv){

    //Check arg count for 2. 1) program name 2) config file path
    if(argc < 2){
        e("Too Few Args");
    }else if(argc > 2) {
        e("Too many Args");
    }

    //Log for arg count / config file
    //printf("Arg Count: %i\nConfig file: %s\n", argc, argv[1]);

    /*************************------Pre Probing Phase------*************************/
    printf("\n********** Pre Probing Phase **********\n");
    //IF we have correct args. Parse config file which should be arg 2
    struct config_settings *settings = parse_config(argv[1]);

    //Initiate tcp connection with server
    unsigned int serverIp = inet_addr(settings->serverIp);
    int comSock = initTcpSock(settings->TcpPortNum, serverIp);

    //Open config file
    FILE *file = fopen(argv[1], "r");
    if(!file){
        e("Failed to open file @argv[1]\n");
    }
    //Send the config file to the server via tcp
    send_file(comSock, file);
    //Log: Config file sent to server
    printf("Config file sent\n");

    //Close TCP connection + config file
    close(comSock);
    fclose(file);

    /*************************------Probing Phase------*************************/
    printf("\n********** Probing Phase **********\n");

    //Create UDP socket
    int udpSock = socket(PF_INET, SOCK_DGRAM, PF_UNSPEC);
    if(udpSock < 0){
        e("Error creating socket for UDP");
    }

    //Create udp addr
    struct sockaddr_in udpAddr;
    int udpAddrSize = sizeof udpAddr;
    bzero(&udpAddr, udpAddrSize);

    //udp addr settings
    udpAddr.sin_family = PF_INET;
    udpAddr.sin_port = htons(settings->destPortUdp);
    udpAddr.sin_addr.s_addr = serverIp;

    //Set udpSock socket settings
    int val = 1;
    if(setsockopt(udpSock, IPPROTO_IP, IP_DONTFRAG, &val, sizeof (val))< 0) {
        e("Failed to set socket option IP_DONTFRAG\n");
    }


    //Create low entropy payload
    unsigned char buffer[settings->UdpPayLoadSize];
    unsigned long bufferSize = sizeof buffer;
    memset(buffer, 35, bufferSize);
    memset(buffer + bufferSize, '\0', 1);

    //Send low entropy payload
    int pTrain =
            sendUdpPacketTrain(settings->UdpPackCount, settings->UdpPayLoadSize,
                               buffer, udpSock, udpAddr);
    if(pTrain < 0){
        e("Failed to send low entropy payload");
    }

    //Log: Waiting Intermission time
    printf("Waiting Intermission time...\n");
    //Wait inter time
    sleep(settings->InterTime);

    //Get high entropy data from file
    FILE * highEntropy = fopen("/Users/lframba/CLionProjects/CS336/CS336_FianlProject_Client/high_entropy_payload.txt", "r");
    if(!highEntropy){
        e("Failed to open High Entropy data file");
    }
    //Load to buffer + close
    fgets(&buffer, bufferSize, highEntropy);
    fclose(highEntropy);
    memset(buffer + bufferSize, '\0', 1);

    //Send High Entropy Data
    pTrain =
            sendUdpPacketTrain(settings->UdpPackCount, settings->UdpPayLoadSize,
                               buffer, udpSock, udpAddr);
    if(pTrain < 0){
        e("Failed to send high entropy payload");
    }

    //We wait an additional Intertime before post probing to allow the server to timeout if needed
    sleep(settings->InterTime);
    /*************************------POST Probing Phase------*************************/
    printf("\n********** Post Probing Phase **********\n");
    comSock = initTcpSock(settings->TcpPortNum, serverIp);
    long results[3];
    recv(comSock, results, sizeof results, 0);
    close(comSock);

    printf("LowEntropy packet train took %ld ms\n", results[0]);
    printf("HighEntropy packet train took %ld ms\n", results[1]);
    if(results[2] == 0){
        long delta = results[1] - results[0];
        printf("Delta time = %li ms\n", delta);
        if(delta > 100){
            printf("Compression Detected!\n");
        }else {
            printf("No Compression Detected!\n");
        }
    }
}



