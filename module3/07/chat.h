#ifndef CHAT_H
#define CHAT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>

#define PORT 51000
#define BUFFER_SIZE 4096
#define MAX_NICKNAME 50
#define MAX_CLIENTS 100
#define MAX_EVENTS 100

#define MSG_JOIN      1
#define MSG_LEAVE     2
#define MSG_CHAT      3
#define MSG_FILE      4
#define MSG_FILE_DATA 5
#define MSG_FILE_END  6
#define MSG_FILE_LIST 7
#define MSG_FILE_GET  8
#define MSG_FILE_INFO 9

typedef struct {
    int type;
    char nickname[MAX_NICKNAME];
    char data[BUFFER_SIZE];
    int data_len;
    int total_size;
    char filename[256];
} Message;

void safe_strcpy(char *dest, const char *src, size_t size);
int send_all(int fd, const void *buf, size_t len);
void set_nonblocking(int fd);

int create_directory(const char *dir_name);

long get_file_size(FILE *file);
int write_file_chunk(FILE *file, const char *data, int data_len);

void set_tcp_nodelay(int fd);
void flush_socket_buffer(int fd);

#endif
