//
// Created by Leonardo Framba on 10/22/22.
//
# include <stdio.h>
# include "/Users/lframba/CLionProjects/CS336/Final-Project/JSON/cJSON.c"
# include "arpa/inet.h"

struct config_settings{
    char *serverIp;
    int sourcePort;
    int destPortUdp;
    char destPortTcpHead;
    char destPortTcpTail;
    int TcpPortNum;
    int UdpPayLoadSize;
    int InterTime;
    long UdpPackCount;
    int Ttl;
};

char *file_to_string(FILE *file) {
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    //Allocate buffer
    char *buffer = malloc(file_size);
    if (!buffer) {
        puts("Error: Unable to allocate memory for buffer");
    }

    //Allocate string
    char *file_string = malloc(file_size * sizeof(char));
    if (!file_string) {
        puts("Error: Unable to allocate memory for file_string");
    }

    //Write contents of file to string
    fread(file_string, sizeof(char), file_size, file);
    fclose(file);

    return file_string;
}

struct config_settings* json_to_struct(cJSON *json){
    struct config_settings *settings = malloc(sizeof (struct config_settings));
    if(!settings){
        puts("Error: Failed to allocate memory for config settings");
        exit(0);
    }

    //TODO there has to be a better way to do this
    //TODO Check for invalid config vals
    settings->serverIp = cJSON_GetObjectItemCaseSensitive(json, "serverIp")->valuestring;
    settings->sourcePort = cJSON_GetObjectItemCaseSensitive(json, "sourcePort")->valueint;
    settings->destPortUdp = cJSON_GetObjectItemCaseSensitive(json, "destPortUdp")->valueint;
    settings->destPortTcpHead = (char) cJSON_GetObjectItemCaseSensitive(json, "destPortTcpHead")->valueint;
    settings->destPortTcpTail = (char) cJSON_GetObjectItemCaseSensitive(json, "destPortTcpTail")->valueint;
    settings->TcpPortNum = cJSON_GetObjectItemCaseSensitive(json, "TcpPortNum")->valueint;

    //Default val 100
    settings->UdpPayLoadSize = cJSON_GetObjectItemCaseSensitive(json, "UdpPayloadSize")->valueint ?
                               cJSON_GetObjectItemCaseSensitive(json, "UdpPayloadSize")->valueint :
                               100;

    //Default val 15
    settings->InterTime = cJSON_GetObjectItemCaseSensitive(json, "InterTime")->valueint ?
                          cJSON_GetObjectItemCaseSensitive(json, "InterTime")->valueint :
                          15;

    //Default 6000
    settings->UdpPackCount = cJSON_GetObjectItemCaseSensitive(json, "UdpPackCount")->valueint ?
                             cJSON_GetObjectItemCaseSensitive(json, "UdpPackCount")->valueint :
                             6000;

    settings->Ttl = cJSON_GetObjectItemCaseSensitive(json, "Ttl")->valueint ?
                    cJSON_GetObjectItemCaseSensitive(json, "Ttl")->valueint :
                    255;

    return settings;
}



struct config_settings* parse_config(char *path) {
    //Invalid path TODO: check vs a defined path name?
    if (!path) {
        puts("Error: Invalid config file path");
        exit(0);
    }

    //Open config file to read
    FILE *config = fopen(path, "r");
    if(!config){
        puts("Error: Unable to open config file");
        exit(0);
    }

    //Convert file to string
    char *file_string = file_to_string(config);
    printf("Successfully loaded config file at: %s\n", path);
    fclose(config);

    //Test print
    printf("%s\n", file_string);

    cJSON *json = cJSON_Parse(file_string);
    if(!json){
        puts("Error: Failed to parse string to json");
        exit(0);
    }

    return json_to_struct(json);
}


