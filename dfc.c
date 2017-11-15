/*
* Jake Mitchell
* Fall 2017 Networks Programming Assignment 3 Distributed file client
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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

//Declarations
struct keyValue *hashTable;




int main(int argc, char **argv) {
    if (argc < 2) {
      printf("missing argument\n");
      exit(-1);
    }
    char* confFile = argv[1];

    hashTable = NULL;
    parse_dfc_config_file(&hashTable, confFile);
    //print_hash(&hashTable);
    printf("Welcome: %s\n", return_value(&hashTable, "Username"));

    char input[LINESIZE];
	int loop = 1;
	while(loop){
        bzero(&input, sizeof(input));
		printf("Enter a command: ");
		fgets(input, LINESIZE, stdin);
		char* command = trimwhitespace(input);
		switch(parse_command(command)){
            case EXIT:
                loop = 0;
                break;
            case LIST:
                printf("list:\n");
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
                add_key_value(hash, host, ipPort);
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
        return f->value;
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
