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

#define STR_VALUE(val) #val
#define STR(name) STR_VALUE(name)

#define MAXFILESIZE 500000
#define LINESIZE 1024
#define PATH_LEN 256
#define MD5_LEN 32
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
void print_hash(struct keyValue**, int);
int parse_command(const char*);
char* send_recieve_from_server(char*, char*, char*, char*);
char* appendString(char*, char* );
void create_listTable();
void find_available_files();
char* send_file_to_servers(char*, char*, char*, char*);
int CalcFileMD5(char*, char*);
char* get_file_part(char*, char*, char*);


//Declarations
struct keyValue *confTable;
struct keyValue *listTable;
struct keyValue *completeTable;
char cwd[LINESIZE];
//hack solution to fix extra character
int first = 1;



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
		char *spaceAt, *filename, *filePath;
		FILE *fp;
		switch(parse_command(command)){
            case EXIT:
                loop = 0;
                break;

            case LIST:
                create_listTable();
                find_available_files();
                print_hash(&completeTable, 0);
                break;

            case GET:;
                spaceAt = strchr(command, ' ');
                *spaceAt = '\0';
                filename = spaceAt+1;

                getcwd(cwd, sizeof(cwd));
                filePath = appendString(cwd, "/");
                filePath = appendString(filePath, filename);

                create_listTable();
                find_available_files();
                //printf("filename:|%s|\n", filename);
                //printf("completeTable for filename:|%s|\n", return_value(&completeTable, filename));
                //print_hash(&completeTable, 1);
                //print_hash(&listTable, 1);
                if (!strcmp(return_value(&completeTable, filename), " ")) {
                    printf("File exists complete on DFS.\n");
                    for (int i = 1; i <=4; i += 1){
                        char iStr[10];
                        sprintf(iStr, "%d", i);
                        char* partFilename = malloc(strlen(input) + 1);
                        strcpy(partFilename, filename);
                        partFilename = appendString(partFilename, ".");
                        partFilename = appendString(partFilename, iStr);
                        //find what server to ask
                        char* server = return_value(&listTable, partFilename);
                        partFilename = appendString(".", partFilename);
                        //printf("filename:%s\n", filename);
                        //printf("server:%s filepart:%s\n", server, partFilename);

                        //get ip and port of wanted server
                        char* ipPort = return_value(&confTable, server);
                        char* const colonAt = strchr(ipPort, ':');
                        *colonAt = '\0';
                        char* ip = ipPort;
                        char* port = colonAt+1;

                        char* part = get_file_part(ip, port, partFilename);

                        //printf("filepart: |%s|\n", part);


                        if (i == 1) {
                            fp = fopen(filePath, "wb+");
                        } else {
                            fp = fopen(filePath, "ab+");
                        }

                        if (fp != NULL) {
                            fwrite(part, strlen(part), 1, fp);
                        }
                        fclose(fp);
                    }


                } else {
                    printf("File incomplete or missing in DFS.\n");
                }

                break;

            case PUT:;
                spaceAt = strchr(command, ' ');
                *spaceAt = '\0';
                filename = spaceAt+1;

                getcwd(cwd, sizeof(cwd));
                filePath = appendString(cwd, "/");
                filePath = appendString(filePath, filename);
                //printf("%s\n", filePath);
                //test if file exists
                if ((fp = fopen(trimwhitespace(filePath), "rb"))) {
                    //calc file
                    int md5num;
                    char md5[MD5_LEN + 1];
                    if (!CalcFileMD5(filePath, md5)) {
                        puts("Error occured!");
                    } else {
                        //printf("Success! MD5 sum is: %s\n", md5);
                        md5num = (md5[(strlen(md5)-1)] - '0')%4;
                        //printf("Success! MD5 sum %% 4 is: %d\n", md5num);
                    }

                    //split file
                    char* splitCmd = "split ";
                    splitCmd = appendString(splitCmd, " -n 4 -d ");
                    splitCmd = appendString(splitCmd, filePath);
                    //printf("split command: %s\n", splitCmd);
                    FILE *split = popen(splitCmd, "r");
                    pclose(split);

                    for (int i = 1; i <=4; i += 1){
                        char iStr[10];
                        sprintf(iStr, "%d", i);
                        char* server = appendString("DFS", iStr);
                        char* ipPort = return_value(&confTable, server);
                        char* const colonAt = strchr(ipPort, ':');
                        *colonAt = '\0';
                        char* ip = ipPort;
                        char* port = colonAt+1;

                        int pairNum = i - md5num;
                        if (pairNum < 1) pairNum += 4;
                        char *part1, *part2, *path1, *path2;
                        switch(pairNum) {
                        case 1:
                            part1 = ".1";
                            part2 = ".2";
                            path1 = "0";
                            path2 = "1";
                            break;
                        case 2:
                            part1 = ".2";
                            part2 = ".3";
                            path1 = "1";
                            path2 = "2";
                            break;

                        case 3:
                            part1 = ".3";
                            part2 = ".4";
                            path1 = "2";
                            path2 = "3";
                            break;

                        case 4:
                            part1 = ".4";
                            part2 = ".1";
                            path1 = "3";
                            path2 = "0";
                            break;
                        }


                        char* partPath = appendString(cwd, "/x0");
                        partPath = appendString(partPath, path1);
                        char* response = send_file_to_servers(ip, port, partPath, appendString(filename, part1));
                        char errorChar = *response;
                        if (errorChar =='~'){
                            response = response + 1;
                            printf("%s\n", response);
                        }
                        char* partPath2 = appendString(cwd, "/x0");
                        partPath2 = appendString(partPath2, path2);
                        response = send_file_to_servers(ip, port, partPath2, appendString(filename, part2));
                        errorChar = *response;
                        if (errorChar =='~'){
                            response = response + 1;
                            printf("%s\n", response);
                        }
                    }
                    fclose(fp);

                } else {
                    printf("File does not exist.\n");
                }

                break;

            default:
                printf("unknown command, please try again\n");
                break;
		}
	}
	printf("goodbye\n");
}



char* get_file_part(char* ip, char* port, char* filename) {
    int sock;
    struct sockaddr_in server;
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1) {
        //printf("Could not create socket");
        return "~Could not create socket";
    }
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons( atoi(port) );

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
        //printf("connect failed. Error\n");
        char* errorMessage = "connection failed.";
        return errorMessage;
    }
    char* sendMessage = return_value(&confTable, "Username");
    sendMessage = appendString(sendMessage, " ");
    sendMessage = appendString(sendMessage, return_value(&confTable, "Password"));
    sendMessage = appendString(sendMessage, " ");
    sendMessage = appendString(sendMessage, "get ");
    sendMessage = appendString(sendMessage, trimwhitespace(filename));
    sendMessage = appendString(sendMessage, " ");
    write(sock , sendMessage , strlen(sendMessage));

    char serverReply[MAXFILESIZE];
    //Receive a reply from the server
    bzero(serverReply, sizeof(serverReply));
    read(sock , serverReply, MAXFILESIZE);

    getcwd(cwd, sizeof(cwd));
    char* filenamePath = appendString(cwd, "/");
    filenamePath = appendString(filenamePath, filename);
    //printf("%s\n", filenamePath);

    /*FILE *fp;
    fp = fopen(filename, "wb+");
    if (fp != NULL) {
        fwrite(serverReply, strlen(serverReply), 1, fp);
    }
    fclose(fp);*/

    //printf("serverReply:%s\n", serverReply);

    close(sock);
    char* serverReplyStr = serverReply;
    return serverReplyStr;
}


char* send_file_to_servers(char* ip, char* port, char* filePath, char* filename) {
    //printf("in send_file_to_servers, filename:%s going to: %s\n", filename, port);
    int sock;
    struct sockaddr_in server;
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1) {
        //printf("Could not create socket");
        return "~Could not create socket";
    }
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons( atoi(port) );

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
        //printf("connect failed. Error\n");
        char* errorMessage = "connection failed.";
        return errorMessage;
    }

    //printf("filename:|%s|\n", filename);
    char* sendMessage = return_value(&confTable, "Username");
    sendMessage = appendString(sendMessage, " ");
    sendMessage = appendString(sendMessage, return_value(&confTable, "Password"));
    sendMessage = appendString(sendMessage, " ");
    sendMessage = appendString(sendMessage, "put .");
    sendMessage = appendString(sendMessage, trimwhitespace(filename));
    sendMessage = appendString(sendMessage, " ");
    //write(sock , sendMessage , strlen(sendMessage));

    //printf("About to open file\n");
    if (first) {
        FILE *fFirst;
        fFirst = fopen(filePath, "rb");
        char buffer[LINESIZE];
        size_t read;
        while ((read = fread(buffer, 1, sizeof(buffer), fFirst)) > 0) {
            //printf("first buffer|%s|\n",buffer);
            //write(sock, buffer, (int)read);

            bzero(buffer, sizeof(buffer));

        }

        fclose(fFirst);
        first = 0;
    }
    FILE *fp;
    if ((fp = fopen(filePath, "rb"))){
        //printf("Filepath:%s\n", filePath);
        //printf("in if\n");

        char buffer[LINESIZE];
        size_t read;
        while ((read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
            //printf("buffer|%s|\n",buffer);
            //write(sock, buffer, (int)read);
            sendMessage = appendString(sendMessage, buffer);

            bzero(buffer, sizeof(buffer));

        }

        write(sock , sendMessage , strlen(sendMessage));
        //printf("sent: |%s|\n", sendMessage);

        fclose(fp);
    } else { //file was not able to be opened
        printf("unable to open:%s.\n", filename);
    }

    close(sock);
    return "file sent.";
}

//from https://stackoverflow.com/questions/3395690/md5sum-of-file-in-linux-c
int CalcFileMD5(char *file_name, char *md5_sum)
{
    #define MD5SUM_CMD_FMT "md5sum %." STR(PATH_LEN) "s 2>/dev/null"
    char cmd[PATH_LEN + sizeof (MD5SUM_CMD_FMT)];
    sprintf(cmd, MD5SUM_CMD_FMT, file_name);
    #undef MD5SUM_CMD_FMT

    FILE *p = popen(cmd, "r");
    if (p == NULL) return 0;

    int i, ch;
    for (i = 0; i < MD5_LEN && isxdigit(ch = fgetc(p)); i++) {
        *md5_sum++ = ch;
    }

    *md5_sum = '\0';
    pclose(p);
    return i == MD5_LEN;
}



void find_available_files() {
    completeTable = NULL;
    struct keyValue *s;
    for(s=listTable; s != NULL; s = s->hh.next) {
        //printf("key: %s, value: %s\n", s->key, s->value);
        char *key = malloc(strlen(s->key) + 1);
        strcpy(key, s->key);
        char * const lastPeriod = strrchr(key, '.');
        *lastPeriod = '\0';
        char* number = lastPeriod+1;

        //if key does not exist in completeTable
        if (strcmp(return_value(&completeTable, key), "")) {
            free(key);
        } else {
            int one = 0, two = 0, three = 0, four = 0;
            //check for all numbers
            char* filenamePart = appendString(key, ".");
            if (return_value(&listTable, appendString(filenamePart, "1")) != "")
                one = 1;
            if (return_value(&listTable, appendString(filenamePart, "2")) != "")
                two = 1;
            if (return_value(&listTable, appendString(filenamePart, "3")) != "")
                three = 1;
            if (return_value(&listTable, appendString(filenamePart, "4")) != "")
                four = 1;
            if (one && two && three && four) {
                add_key_value(&completeTable, key, " ");
            } else {
                add_key_value(&completeTable, key, " [incomplete]");
            }
        }
    }
}


void create_listTable(){
    listTable = NULL;
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

        char* response = send_recieve_from_server(server, ip, port, sendMessage);
        char errorChar = *response;

        if (errorChar !='~'){
            //printf("|%s|\n", response);
            size_t len = 0;
            FILE *responseStream;
            char *responseLine;
            ssize_t read;
            char responseBuffer[LINESIZE];
            strcpy(responseBuffer, response);
            responseStream = fmemopen(responseBuffer, strlen(responseBuffer), "r");

            //Get serverName
            getline(&responseLine, &len, responseStream);
            char serverNameBuffer[20];
            strcpy(serverNameBuffer, trimwhitespace(responseLine));
            //remove leading / and trailing :
            char* serverName = serverNameBuffer;
            serverName = serverName+1;
            serverName[strlen(serverName)-1] = '\0';
            while ((read = getline(&responseLine, &len, responseStream)) != -1){
                add_key_value(&listTable, trimwhitespace(responseLine+1), serverName);
                //printf("Got %s", responseLine);
                bzero(responseLine, sizeof(responseLine));
            }
        } else {
            response = response + 1;
            printf("%s\n", response);
        }
    }
    //print_hash(&listTable);
}


char* appendString(char* str1, char* str2) {
    char * str3 = (char *) malloc(1 + strlen(str1)+ strlen(str2) );
    strcpy(str3, str1);
    strcat(str3, str2);
    return str3;
}


char* send_recieve_from_server(char* serverName, char* ip, char* port, char* message) {
    int sock;
    struct sockaddr_in server;
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1) {
        //printf("Could not create socket");
        return "~Could not create socket";
    }
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons( atoi(port) );

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
        //printf("connect failed. Error\n");
        char* errorMessage = "connect to ";
        errorMessage = appendString(errorMessage, serverName);
        errorMessage = appendString(errorMessage, " failed.");
        return errorMessage;
    }
    if( write(sock , message , strlen(message)) < 0) {
        //printf("Send failed\n");
        return "~Send failed";
    }
    char serverReply[LINESIZE];
    //Receive a reply from the server
    bzero(serverReply, sizeof(serverReply));
    read(sock , serverReply, LINESIZE);

    close(sock);

    char* outMessage = serverReply;
    return outMessage;
}


int parse_command(const char* input) {
    //printf("|%s|\n", input);
    char * copy = malloc(strlen(input) + 1);
    strcpy(copy, input);
    if (strcmp(input, "")) {
        copy = strtok(copy, " ");
    }
    if (!strcasecmp(copy, "EXIT")){
        return EXIT;
    } else if (!strcasecmp(copy,"LIST")){
        return LIST;
    } else if (!strcasecmp(copy,"GET")){
        return GET;
    } else if (!strcasecmp(copy,"PUT")){
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

void print_hash(struct keyValue **hash, int showKeyValue) {
    struct keyValue *s;
    for(s=*hash; s != NULL; s = s->hh.next) {
        if (showKeyValue) {
            printf("key: |%s|, value: |%s|\n", s->key, s->value);
        } else {
            printf("%s %s\n", s->key, s->value);
        }
    }
}
