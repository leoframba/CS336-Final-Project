# include <stdio.h>
# include <stdlib.h>
# include <strings.h>
#include <netinet/in.h>
#include "Config_Parser.c"
# include "/Users/lframba/CLionProjects/CS336/Final-Project/JSON/cJSON.h"



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

    //Parse config setting file into a settings struct
    struct config_settings *settings = parse_config(argv[1]);




    int sock, connId;
    struct sockaddr_in sAddr, cli;

    //Create Socket -> socket(int domain, int type, int protocol)
    sock = socket(PF_INET, SOCK_STREAM, PF_UNSPEC);

    if(sock == -1){
        puts("Failed to create socket!");
        exit(0);
    }else{
        puts("Successfully created socket!");
    }
    //memset but assumes starting val is 0
    //Set all values in mem to null
    bzero(&sAddr, sizeof (sAddr));
}
