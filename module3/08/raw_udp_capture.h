#ifndef RAW_UDP_CAPTURE_H
#define RAW_UDP_CAPTURE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <errno.h>
#include <ctype.h>

#define BUFFER_SIZE 65536
#define MAX_FILTERS 10
#define FILTER_NAME_SIZE 50
#define MAX_MAC_LEN 18

typedef struct {
    struct timeval timestamp;
    unsigned char src_mac[6];
    unsigned char dst_mac[6];
    char src_ip[INET_ADDRSTRLEN];
    char dst_ip[INET_ADDRSTRLEN];
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t udp_length;
    unsigned char *payload;
    uint16_t payload_length;
} captured_packet_t;

typedef struct {
    char name[FILTER_NAME_SIZE];
    uint16_t src_port;
    uint16_t dst_port;
    char src_ip[INET_ADDRSTRLEN];
    char dst_ip[INET_ADDRSTRLEN];
    int use_src_port;
    int use_dst_port;
    int use_src_ip;
    int use_dst_ip;
} filter_t;

extern int running;
extern int packet_count;
extern captured_packet_t *packets;
extern int max_packets;
extern char output_filename[256];
extern filter_t filters[MAX_FILTERS];
extern int filter_count;
extern int current_filter_index;

void init_filters(void);
int apply_filter(const struct iphdr *ip_header, const struct udphdr *udp_header);
void add_filter(const char *name, uint16_t src_port, uint16_t dst_port, 
                const char *src_ip, const char *dst_ip);

int init_raw_socket(void);
void capture_packets(int raw_sock);
void process_packet(const unsigned char *buffer, int length);
void add_packet(struct timeval *tv, const unsigned char *buffer, int length,
                const struct iphdr *ip, const struct udphdr *udp);

void print_packet(const captured_packet_t *pkt, int index);
void print_all_packets(void);
void save_to_file(const char *filename);
void print_mac(const unsigned char *mac, char *buffer);

void get_mac_address(const unsigned char *buffer, unsigned char *src_mac, 
                     unsigned char *dst_mac);
void signal_handler(int sig);
void cleanup(void);

#endif
