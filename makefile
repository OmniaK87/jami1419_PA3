CC = gcc

all: dfs dfc

dfs:
	$(CC) -pthread -g -o dfs -lm dfs.c

dfc:
	$(CC) -pthread -g -o dfc -lm dfc.c

clean:
	rm -f dfs dfc
