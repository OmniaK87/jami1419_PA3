/*
* Jake Mitchell
* Fall 2017 Networks Programming Assignment 3 Distibuted file server
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
void parse_dfs_config_file(struct keyValue**, char*);
char* return_value(struct keyValue**, char*);
void print_hash(struct keyValue **);
void *connection_handler(void *);

//Declarations
struct keyValue *hashTable;
int socket_desc;


int main(int argc, char **argv) {
    if (argc < 3) {
      printf("missing argument\n");
      exit(-1);
    }

    hashTable = NULL;
    parse_dfs_config_file(&hashTable, "dfs.conf");

    int client_sock , *new_sock;
    struct sockaddr_in server , client;
    char* serverName = argv[1];
    char filePath[LINESIZE];
    getcwd(filePath, sizeof(filePath));
    strcat(filePath, serverName);
    printf("%s\n", filePath);
    int port = atoi(argv[2]);

    //base code from http://www.binarytides.com/server-client-example-c-sockets-linux/
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1){ printf("Could not create socket");    }

    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        printf("bind failed. Error\nn");

        return 1;
    }
    listen(socket_desc , 3);

    //Accept and incoming connection
    int c = sizeof(struct sockaddr_in);
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = client_sock;

        if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }
    }

    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }

    close(socket_desc);
    printf("end\n");
}



//Functions
void parse_dfs_config_file(struct keyValue **hash, char* file){
    char * line = NULL;
    FILE *confFile;
    size_t len = 0;
    ssize_t read;

    confFile = fopen(file, "r");
    if (confFile == NULL) {
        printf("Unable to open the conf file.\n");
        exit(EXIT_FAILURE);
    }

    while ((read = getline(&line, &len, confFile)) != -1) {
        if(line[0] != '#'){
            //printf("%s", line);
            char* second = strchr(line, ' ');
            size_t lengthOfFirst = second - line;
            char* first = (char*)malloc((lengthOfFirst + 1)*sizeof(char));
            strncpy(first, line, lengthOfFirst);

            first = trimwhitespace(first);
            second = trimwhitespace(second);

            add_key_value(hash, first, second);
        }
    }
    /*struct keyValue *f = findKey(hash, ".jpg");
    printf("%s\n", f->value);*/
    fclose(confFile);
}

char* return_value(struct keyValue **hash, char* key) {
    struct keyValue *f = findKey(hash, key);
    if (f != NULL) {
        return f->value;
    } else {
        return "";
    }
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

void print_hash(struct keyValue **hash) {
    struct keyValue *s;
    for(s=*hash; s != NULL; s = s->hh.next) {
        printf("key: %s, value: %s\n", s->key, s->value);
    }
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;

    printf("Connection Recieved\n");

    //Free the socket pointer
    free(socket_desc);
    close(sock);

    printf("closed\n");
    return 0;
}



