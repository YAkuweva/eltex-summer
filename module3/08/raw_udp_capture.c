#include "raw_udp_capture.h"

int running = 1;
int packet_count = 0;
captured_packet_t *packets = NULL;
int max_packets = 1000;
char output_filename[256] = "packets.txt";

filter_t filters[MAX_FILTERS];
int filter_count = 0;
int current_filter_index = 0;

void signal_handler(int sig) {
    (void)sig;
    running = 0;
    printf("\n\n   Stopped\n");
}

void init_filters() {
    filter_count = 0;
    
    add_filter("Chat messages (p 51000)", 51000, 51000, NULL, NULL);
    
    add_filter("DNS (p 53)", 53, 53, NULL, NULL);
    
    add_filter("NTP (p 123)", 123, 123, NULL, NULL);
    
}

void add_filter(const char *name, uint16_t src_port, uint16_t dst_port, 
                const char *src_ip, const char *dst_ip) {
    if (filter_count >= MAX_FILTERS) return;
    
    strncpy(filters[filter_count].name, name, FILTER_NAME_SIZE - 1);
    filters[filter_count].name[FILTER_NAME_SIZE - 1] = '\0';
    
    filters[filter_count].src_port = src_port;
    filters[filter_count].dst_port = dst_port;
    
    filters[filter_count].use_src_port = (src_port != 0);
    filters[filter_count].use_dst_port = (dst_port != 0);
    
    if (src_ip != NULL) {
        strncpy(filters[filter_count].src_ip, src_ip, INET_ADDRSTRLEN - 1);
        filters[filter_count].src_ip[INET_ADDRSTRLEN - 1] = '\0';
        filters[filter_count].use_src_ip = 1;
    } else {
        filters[filter_count].use_src_ip = 0;
    }
    
    if (dst_ip != NULL) {
        strncpy(filters[filter_count].dst_ip, dst_ip, INET_ADDRSTRLEN - 1);
        filters[filter_count].dst_ip[INET_ADDRSTRLEN - 1] = '\0';
        filters[filter_count].use_dst_ip = 1;
    } else {
        filters[filter_count].use_dst_ip = 0;
    }
    
    filter_count++;
}

int apply_filter(const struct iphdr *ip_header, const struct udphdr *udp_header) {
    if (filter_count == 0) return 1;
    
    filter_t *f = &filters[current_filter_index];
    
    if (!f->use_src_port && !f->use_dst_port && 
        !f->use_src_ip && !f->use_dst_ip) {
        return 1;
    }
    
    uint16_t src_port = ntohs(udp_header->source);
    uint16_t dst_port = ntohs(udp_header->dest);
    
    if (f->src_port == f->dst_port && f->use_src_port && f->use_dst_port) {
        if (src_port == f->src_port || dst_port == f->dst_port) {
        } else {
            return 0;
        }
    } else {
        if (f->use_src_port && src_port != f->src_port) {
            return 0;
        }
        if (f->use_dst_port && dst_port != f->dst_port) {
            return 0;
        }
    }
    
    char src_ip[INET_ADDRSTRLEN];
    char dst_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip_header->saddr, src_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &ip_header->daddr, dst_ip, INET_ADDRSTRLEN);
    
    if (f->use_src_ip && strcmp(src_ip, f->src_ip) != 0) {
        return 0;
    }
    
    if (f->use_dst_ip && strcmp(dst_ip, f->dst_ip) != 0) {
        return 0;
    }
    
    return 1;
}

int init_raw_socket() {
    int raw_sock;
    
    raw_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (raw_sock < 0) {
        perror("socket creation failed");
        printf("Error: Raw socket creation\n");
        return -1;
    }
    
    return raw_sock;
}

void get_mac_address(const unsigned char *buffer, unsigned char *src_mac, 
                     unsigned char *dst_mac) {
    struct ether_header *eth = (struct ether_header *)buffer;
    memcpy(dst_mac, eth->ether_dhost, 6);
    memcpy(src_mac, eth->ether_shost, 6);
}

void print_mac(const unsigned char *mac, char *buffer) {
    sprintf(buffer, "%02x:%02x:%02x:%02x:%02x:%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void process_packet(const unsigned char *buffer, int length) {
    struct ether_header *eth = (struct ether_header *)buffer;
    
    if (ntohs(eth->ether_type) != ETHERTYPE_IP) {
        return;
    }
    
    struct iphdr *ip = (struct iphdr *)(buffer + sizeof(struct ether_header));
    
    if (ip->protocol != IPPROTO_UDP) {
        return;
    }
    
    int ip_header_len = ip->ihl * 4;
    struct udphdr *udp = (struct udphdr *)((unsigned char *)ip + ip_header_len);
    
    if (!apply_filter(ip, udp)) {
        return;
    }
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    add_packet(&tv, buffer, length, ip, udp);
}

void add_packet(struct timeval *tv, const unsigned char *buffer, int length,
                const struct iphdr *ip, const struct udphdr *udp) {
    if (packet_count >= max_packets) {
        printf("Maximum packet count reached. Stopping capture.\n");
        running = 0;
        return;
    }
    
    captured_packet_t *pkt = &packets[packet_count];
    
    pkt->timestamp = *tv;
    
    get_mac_address(buffer, pkt->src_mac, pkt->dst_mac);
    
    inet_ntop(AF_INET, &ip->saddr, pkt->src_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &ip->daddr, pkt->dst_ip, INET_ADDRSTRLEN);
    
    pkt->src_port = ntohs(udp->source);
    pkt->dst_port = ntohs(udp->dest);
    pkt->udp_length = ntohs(udp->len);
    
    int ip_header_len = ip->ihl * 4;
    int udp_header_len = sizeof(struct udphdr);
    int payload_offset = sizeof(struct ether_header) + ip_header_len + udp_header_len;
    pkt->payload_length = length - payload_offset;
    
    if (pkt->payload_length > 0) {
        pkt->payload = malloc(pkt->payload_length);
        if (pkt->payload) {
            memcpy(pkt->payload, buffer + payload_offset, pkt->payload_length);
        }
    } else {
        pkt->payload = NULL;
    }
    
    packet_count++;
}

void capture_packets(int raw_sock) {
    unsigned char buffer[BUFFER_SIZE];
    int bytes_received;
    
    printf("\n   Starting packet capture \n");
    printf("Max packets: %d\n\n", max_packets);
    
    packets = malloc(max_packets * sizeof(captured_packet_t));
    if (!packets) {
        perror("malloc failed");
        return;
    }
    memset(packets, 0, max_packets * sizeof(captured_packet_t));
    
    while (running && packet_count < max_packets) {
        bytes_received = recv(raw_sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("recv error");
            break;
        }
        
        if (bytes_received > 0) {
            process_packet(buffer, bytes_received);
            
            if (packet_count % 10 == 0 && packet_count > 0) {
                printf("Captured %d packets\r", packet_count);
                fflush(stdout);
            }
        }
    }
    
    printf("\nCapture finished. Total packets: %d\n", packet_count);
}

void print_packet(const captured_packet_t *pkt, int index) {
    char mac_src[MAX_MAC_LEN];
    char mac_dst[MAX_MAC_LEN];
    
    print_mac(pkt->src_mac, mac_src);
    print_mac(pkt->dst_mac, mac_dst);
    
    static struct timeval start_time = {0, 0};
    if (index == 0) {
        start_time = pkt->timestamp;
    }
    
    double elapsed = (pkt->timestamp.tv_sec - start_time.tv_sec) + 
                     (pkt->timestamp.tv_usec - start_time.tv_usec) / 1000000.0;
    
    printf("\n   Packet #%d:\n", index + 1);
    printf("Time: %.6f seconds\n", elapsed);
    printf("MAC src: %s -> dst: %s\n", mac_src, mac_dst);
    printf("IP src: %s -> dst: %s\n", pkt->src_ip, pkt->dst_ip);
    printf("UDP port: %d -> %d\n", pkt->src_port, pkt->dst_port);
    printf("UDP length: %d bytes\n", pkt->udp_length);
    printf("Payload: %d bytes\n", pkt->payload_length);
    
    if (pkt->payload_length > 0 && pkt->payload) {
        printf("Data (hex): ");
        for (int i = 0; i < pkt->payload_length && i < 32; i++) {
            printf("%02x ", pkt->payload[i]);
        }
        if (pkt->payload_length > 32) {
            printf("...");
        }
        printf("\n");
        
        printf("Data (text): ");
        int printable = 0;
        for (int i = 0; i < pkt->payload_length && i < 64; i++) {
            if (isprint(pkt->payload[i])) {
                printf("%c", pkt->payload[i]);
                printable = 1;
            } else if (pkt->payload[i] == '\0') {
                break;
            } else {
                if (printable) printf(".");
                printable = 0;
            }
        }
        printf("\n");
    }
}

void print_all_packets() {
    if (packet_count == 0) {
        printf("No packets captured.\n");
        return;
    }
    
    printf("\n   Captured Packets (%d)\n", packet_count);
    for (int i = 0; i < packet_count; i++) {
        print_packet(&packets[i], i);
    }
}

void save_to_file(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("fopen failed");
        return;
    }
    
    time_t now = time(NULL);
    fprintf(file, "   UDP Packet Capture Report\n");
    fprintf(file, "Total packets: %d\n", packet_count);
    fprintf(file, "Capture date: %s", ctime(&now));
    fprintf(file, "\n");
    
    for (int i = 0; i < packet_count; i++) {
        captured_packet_t *pkt = &packets[i];
        char mac_src[MAX_MAC_LEN];
        char mac_dst[MAX_MAC_LEN];
        
        print_mac(pkt->src_mac, mac_src);
        print_mac(pkt->dst_mac, mac_dst);
        
        fprintf(file, "   Packet #%d:\n", i + 1);
        fprintf(file, "Time: %ld.%06ld\n", 
                pkt->timestamp.tv_sec, pkt->timestamp.tv_usec);
        fprintf(file, "MAC: %s -> %s\n", mac_src, mac_dst);
        fprintf(file, "IP: %s -> %s\n", pkt->src_ip, pkt->dst_ip);
        fprintf(file, "UDP: %d -> %d\n", pkt->src_port, pkt->dst_port);
        fprintf(file, "Length: %d bytes\n", pkt->udp_length);
        fprintf(file, "Payload: %d bytes\n", pkt->payload_length);
        
        if (pkt->payload_length > 0 && pkt->payload) {
            fprintf(file, "Data (hex): ");
            for (int j = 0; j < pkt->payload_length && j < 64; j++) {
                fprintf(file, "%02x ", pkt->payload[j]);
            }
            if (pkt->payload_length > 64) {
                fprintf(file, "...");
            }
            fprintf(file, "\n");
            
            fprintf(file, "Data (text): ");
            for (int j = 0; j < pkt->payload_length && j < 64; j++) {
                if (isprint(pkt->payload[j])) {
                    fprintf(file, "%c", pkt->payload[j]);
                }
            }
            fprintf(file, "\n");
        }
        fprintf(file, "\n");
    }
    
    fclose(file);
    printf("\nData saved to: %s\n", filename);
}

void cleanup() {
    if (packets) {
        for (int i = 0; i < packet_count; i++) {
            if (packets[i].payload) {
                free(packets[i].payload);
                packets[i].payload = NULL;
            }
        }
        free(packets);
        packets = NULL;
    }
}
