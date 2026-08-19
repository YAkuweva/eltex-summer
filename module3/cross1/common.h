#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define BUFFER_SIZE 1024
#define PORT 51000
#define MAX_CLIENTS 100

struct udp_header {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
};

struct client_info {
    struct sockaddr_in addr;
    int counter;
    int active;
};

uint16_t udp_checksum(struct iphdr *ip_header, struct udphdr *udp_header, 
                       const char *data, int data_len);

#endif
