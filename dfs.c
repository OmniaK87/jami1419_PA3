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
#include <sys/stat.h>

#include "uthash.h"

#define LINESIZE 1024
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
void parse_dfs_config_file(struct keyValue**, char*);
char* return_value(struct keyValue**, char*);
void print_hash(struct keyValue **);
void *connection_handler(void *);
char* appendString(char*, char*);
void certify_user(char**, char**, char**, char**, int*);
int parse_command(const char*);

//Declarations
struct keyValue *confTable;
int socket_desc;
char* serverName;
char filePath[LINESIZE];


int main(int argc, char **argv) {
    if (argc < 3) {
      printf("missing argument\n");
      exit(-1);
    }
    serverName = argv[1];

    confTable = NULL;
    parse_dfs_config_file(&confTable, "dfs.conf");

    int client_sock , *new_sock;
    struct sockaddr_in server , client;
    char* serverName = argv[1];
    getcwd(filePath, sizeof(filePath));
    strcat(filePath, serverName);
    printf("%s\n", filePath);
    int result = mkdir(filePath, 0777);
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
char* appendString(char* str1, char* str2) {
    char * str3 = (char *) malloc(1 + strlen(str1)+ strlen(str2) );
    strcpy(str3, str1);
    strcat(str3, str2);
    return str3;
}


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
        char * copy = malloc(strlen(f->value) + 1);
        strcpy(copy, f->value);
        return copy;
    } else {
        return "";
    }
}


void add_key_value(struct keyValue **hash, char* keyIn, char* valueIn) {
    struct keyValue *f = findKey(hash, keyIn);
    if (f != NULL) {
        HASH_DEL(*hash, f);
        free(f);
    }
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

void certify_user(char** mess, char** us, char** ps, char** cmd, int* cert) {
    char* spaceAt = strchr(*mess, ' ');
    *spaceAt = '\0';
    *us = *mess;
    char* passPlus = spaceAt+1;
    spaceAt = strchr(passPlus, ' ');
    *spaceAt = '\0';
    *ps = passPlus;
    *cmd = spaceAt+1;
    char* userConfPass = return_value(&confTable, *us);
    if (!strcmp(userConfPass, *ps)) {
        *cert = 1;
    } else {
        *cert = 0;
    }
}

int parse_command(const char* input) {
    //printf("|%s|\n", input);
    char * copy = malloc(strlen(input) + 1);
    strcpy(copy, input);
    copy = strtok(copy, " ");
    if (!strcasecmp(copy, "LIST")){
        return LIST;
    } else if (!strcasecmp(copy,"GET")){
        return GET;
    } else if (!strcasecmp(copy,"PUT")){
        return PUT;
    } else {
        return -1;
    }
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
    int result = mkdir(filePath, 0777);
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size, certified;
    char *message, *user, *pass, *command, client_message[2000];

    //Receive a message from client
    while( (read_size = read(sock , client_message , LINESIZE)) > 0 ) {
        char*  outMessage;
        char* message = trimwhitespace(client_message);
        FILE *fp;
        printf("Client message: ");
        printf("%s\n", message);

        certify_user(&message, &user, &pass, &command, &certified);
        if (certified) {
            char* userFilePath = filePath;
            userFilePath = appendString(userFilePath, "/");
            userFilePath = appendString(userFilePath, user);
            char *filename, *spaceAt, *filenamePath;
            switch(parse_command(command)){
            case LIST:;
                outMessage = appendString(serverName, ":\n");
                char path[LINESIZE];

                //make sure the user's directory exists
                int result = mkdir(userFilePath, 0777);

                char* listCmd = "/bin/ls ";
                listCmd = appendString(listCmd, userFilePath);
                listCmd = appendString(listCmd, " -a");
                //printf("listCmd:%s\n", listCmd);

                FILE *ls = popen(listCmd, "r");
                if (ls != NULL) {
                    while (fgets(path, LINESIZE, ls) != NULL) {
                        //printf("%s", path);
                        if (strcmp(trimwhitespace(path), ".") && strcmp(trimwhitespace(path), "..")) {
                            outMessage = appendString(outMessage, path);
                            outMessage = appendString(outMessage, "\n");
                        }
                    }
                    pclose(ls);
                } else {
                    printf("Unable to list directory.\n");
                    outMessage = appendString(outMessage, "Unable to list directory.");
                }

                outMessage = trimwhitespace(outMessage);
                write(sock , outMessage, strlen(outMessage));
                printf("Response:\n%s\n", outMessage);

                break;
            case GET:
                outMessage = appendString(serverName, ":\n");

                spaceAt = strchr(command, ' ');
                *spaceAt = '\0';
                filename = spaceAt+1;
                filenamePath = userFilePath;
                filenamePath = appendString(filenamePath, "/");
                filenamePath = appendString(filenamePath, filename);
                printf("filename: %s\n", filename);
                printf("filenamePath: %s\n", filenamePath);

                if ((fp = fopen(filenamePath, "rb"))){
                    char * line = NULL;
                    size_t len = 0;

                    char buffer[LINESIZE];
                    size_t read;
                    while ((read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
                        write(sock, buffer, (int)read);
                    }

                    fclose(fp);
                } else { //file was not able to be opened
                    printf("unable to open:%s.\n", filename);
                }


                outMessage = trimwhitespace(outMessage);
                write(sock , outMessage, strlen(outMessage));
                printf("Response:\n%s\n", outMessage);
                break;
            case PUT:;
                spaceAt = strchr(command, ' ');
                *spaceAt = '\0';
                char* filenamePlus = spaceAt+1;
                spaceAt = strchr(filenamePlus, ' ');
                *spaceAt = '\0';
                filename = filenamePlus;
                char* file = spaceAt+1;
                filenamePath = userFilePath;
                filenamePath = appendString(filenamePath, "/");
                filenamePath = appendString(filenamePath, filename);

                FILE *fp;
                fp = fopen(filenamePath, "wb+");
                if (fp != NULL) {
                    fwrite(file, strlen(file), 1, fp);
                }
                fclose(fp);
                printf("Saved file to:%s\n", filenamePath);

                break;
            default:
                outMessage = appendString(serverName, ":");
                outMessage = appendString(outMessage, "unknown command.");
                write(sock , outMessage, strlen(outMessage));
            }
        }
        else {
            //Send a message back to client
            outMessage = appendString(serverName, ":");
            outMessage = appendString(outMessage, "Invalid Username/Password. Please try again.");
            write(sock , outMessage, strlen(outMessage));
        }
    }

    if(read_size == 0) {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1) {
        perror("recv failed");
    }
    printf("\n");

    //Free the socket pointer
    free(socket_desc);

    return 0;
}



