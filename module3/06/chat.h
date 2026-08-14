#ifndef CHAT_H
#define CHAT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>

#define PORT 51000
#define BUFFER_SIZE 1024
#define MAX_NICKNAME 50

extern int sockfd;
extern struct sockaddr_in broadcast_addr;
extern char nickname[MAX_NICKNAME];

#endif 
