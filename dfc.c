/*
* Jake Mitchell
* Fall 2017 Networks Programming Assignment 3 Distributed file client
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h> //for threading , link with lpthread
#include <signal.h>
#include "uthash.h"

#define LINESIZE 1024
#define EXIT 1
#define LIST 2
#define GET 3
#define PUT 4



//Structs
struct keyValue {
    char key[LINESIZE];
    char value[LINESIZE];
    UT_hash_handle hh;
};


//Funnction Delcarations
void add_key_value(struct keyValue** , char*, char*);
struct keyValue *findKey(struct keyValue**, char*);
char* trimwhitespace(char*);
void parse_dfc_config_file(struct keyValue**, char*);
char* return_value(struct keyValue**, char*);
void print_hash(struct keyValue **hash);
int parse_command(const char*);
char* send_recieve_from_server(char*, char*, char*);
char* appendString(char*, char* );

//Declarations
//
struct keyValue *confTable;
struct keyValue *listTable;




int main(int argc, char **argv) {
    if (argc < 2) {
      printf("missing argument\n");
      exit(-1);
    }
    char* confFile = argv[1];

    confTable = NULL;
    parse_dfc_config_file(&confTable, confFile);
    //print_hash(&hashTable);
    printf("Welcome: %s\n", return_value(&confTable, "Username"));

    char input[LINESIZE];

	int loop = 1;
	while(loop){
        bzero(&input, sizeof(input));
		printf("\nEnter a command: ");
		fgets(input, LINESIZE, stdin);
		char* command = trimwhitespace(input);
		switch(parse_command(command)){
            case EXIT:
                loop = 0;
                break;

            case LIST:;
                printf("list:\n");
                char* sendMessage = return_value(&confTable, "Username");
                sendMessage = appendString(sendMessage, " ");
                sendMessage = appendString(sendMessage, return_value(&confTable, "Password"));
                sendMessage = appendString(sendMessage, " ");
                sendMessage = appendString(sendMessage, "list");
                for (int i = 1; i <=4; i += 1){
                    char iStr[10];
                    sprintf(iStr, "%d", i);
                    char* server = appendString("DFS", iStr);
                    char* ipPort = return_value(&confTable, server);
                    char* const colonAt = strchr(ipPort, ':');
                    *colonAt = '\0';
                    char* ip = ipPort;
                    char* port = colonAt+1;

                    char* response = send_recieve_from_server(ip, port, sendMessage);
                    printf("%s\n", response);
                }
                break;

            case GET:
                printf("get:\n");
                break;

            case PUT:
                printf("put:\n");
                break;

            case -1:
                printf("unknown command, please try again\n");
                break;
		}
	}
	printf("goodbye\n");
}

char* appendString(char* str1, char* str2) {
    char * str3 = (char *) malloc(1 + strlen(str1)+ strlen(str2) );
    strcpy(str3, str1);
    strcat(str3, str2);
    return str3;
}


char* send_recieve_from_server(char* ip, char* port, char* message) {
    int sock;
    struct sockaddr_in server;
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1) {
        //printf("Could not create socket");
        return "Could not create socket";
    }
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons( atoi(port) );

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
        //printf("connect failed. Error\n");
        char* errorMessage = "connect to ";
        errorMessage = appendString(errorMessage, ip);
        errorMessage = appendString(errorMessage, ":");
        errorMessage = appendString(errorMessage, port);
        errorMessage = appendString(errorMessage, " failed.");
        return errorMessage;
    }
    if( write(sock , message , strlen(message)) < 0) {
        //printf("Send failed\n");
        return "Send failed";
    }
    char serverReply[LINESIZE];
    //Receive a reply from the server
    bzero(serverReply, sizeof(serverReply));
    read(sock , serverReply, LINESIZE);

    close(sock);

    char* outMessage = serverReply;
    return outMessage;
}


//Functions
int parse_command(const char* input) {
    //printf("|%s|\n", input);
    if (!strcasecmp(input, "EXIT")){
        return EXIT;
    } else if (!strcasecmp(input,"LIST")){
        return LIST;
    } else if (!strcasecmp(input,"GET")){
        return GET;
    } else if (!strcasecmp(input,"PUT")){
        return PUT;
    } else {
        return -1;
    }
}


void parse_dfc_config_file(struct keyValue **hash, char* file){
    char * line = NULL;
    FILE *confFile;
    size_t len = 0;
    ssize_t read;

    confFile = fopen(file, "r");
    if (confFile == NULL)
        exit(EXIT_FAILURE);
    while ((read = getline(&line, &len, confFile)) != -1) {
        if(line[0] != '#'){
            //printf("%s", line);
            char* second = strchr(line, ' ');
            size_t lengthOfFirst = second - line;
            char* first = (char*)malloc((lengthOfFirst + 1)*sizeof(char));
            strncpy(first, line, lengthOfFirst);

            first = trimwhitespace(first);
            second = trimwhitespace(second);

            if (!strcmp(first, "Server")){
                char* ipPort = strchr(second, ' ');
                size_t lengthOfHost = ipPort - second;
                char* host = (char*)malloc((lengthOfHost + 1)*sizeof(char));
                strncpy(host, second, lengthOfHost);
                add_key_value(hash, host, trimwhitespace(ipPort));
            } else {
                add_key_value(hash, first, second);
            }
        }
    }
    /*struct keyValue *f = findKey(hash, ".jpg");
    printf("%s\n", f->value);*/
    fclose(confFile);
}


void add_key_value(struct keyValue **hash, char* keyIn, char* valueIn) {
    struct keyValue *s;
    HASH_FIND_INT(*hash, &keyIn, s);  //id already in the hash?
    if (s==NULL) {
        s = malloc(sizeof(struct keyValue));
        strcpy(s->key, keyIn);
        strcpy(s->value, valueIn);
        HASH_ADD_STR(*hash, key, s);
    }
}

struct keyValue *findKey(struct keyValue **hash, char* keyId) {
    struct keyValue *s;
    HASH_FIND_STR(*hash, keyId, s);
    return s;
};

//from https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
char *trimwhitespace(char *str)
{
  char *end;

  while(isspace((unsigned char)*str)) str++; // Trim leading space

  if(*str == 0)  // All spaces?
    return str;

  end = str + strlen(str) - 1; // Trim trailing space
  while(end > str && isspace((unsigned char)*end)) end--;

  *(end+1) = 0; // Write new null terminator

  return str;
}

char* return_value(struct keyValue **hash, char* key) {
    struct keyValue *f = findKey(hash, key);
    if (f != NULL) {
        char * copy = malloc(strlen(f->value) + 1);
        strcpy(copy, f->value);
        return copy;
    } else {
        return "";
    }
}

void print_hash(struct keyValue **hash) {
    struct keyValue *s;
    for(s=*hash; s != NULL; s = s->hh.next) {
        printf("key: %s, value: %s\n", s->key, s->value);
    }
}
