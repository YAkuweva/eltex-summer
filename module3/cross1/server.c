#include "common.h"

struct client_info clients[MAX_CLIENTS];
int raw_sock;
int running = 1;
struct sockaddr_in server_addr;

void signal_handler(int signo) {
    (void)signo;
    printf("\nServer shutting down...\n");
    running = 0;
}

struct client_info* find_client(struct sockaddr_in *addr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && 
            clients[i].addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
            clients[i].addr.sin_port == addr->sin_port) {
            return &clients[i];
        }
    }
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].addr = *addr;
            clients[i].counter = 1;
            clients[i].active = 1;
            return &clients[i];
        }
    }
    return NULL;
}

void remove_client(struct sockaddr_in *addr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && 
            clients[i].addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
            clients[i].addr.sin_port == addr->sin_port) {
            clients[i].active = 0;
            clients[i].counter = 0;
            printf("Client %s:%d disconnected, counter reset\n",
                   inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
            return;
        }
    }
}

int build_udp_packet(char *packet, struct sockaddr_in *src, 
                      struct sockaddr_in *dst, const char *data, int data_len) {
    struct iphdr *ip = (struct iphdr*)packet;
    struct udphdr *udp = (struct udphdr*)(packet + sizeof(struct iphdr));
    char *payload = packet + sizeof(struct iphdr) + sizeof(struct udphdr);
    
    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + data_len);
    ip->id = htons(rand() % 65535);
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = IPPROTO_UDP;
    ip->check = 0;
    ip->saddr = src->sin_addr.s_addr;
    ip->daddr = dst->sin_addr.s_addr;
    
    udp->source = src->sin_port;
    udp->dest = dst->sin_port;
    udp->len = htons(sizeof(struct udphdr) + data_len);
    udp->check = 0;
    
    memcpy(payload, data, data_len);
    
    ip->check = 0;
    udp->check = udp_checksum(ip, udp, data, data_len);
    
    return sizeof(struct iphdr) + sizeof(struct udphdr) + data_len;
}

int main(void) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char packet[4096];
    int bytes;
    
    memset(clients, 0, sizeof(clients));
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    if ((raw_sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) < 0) {
        perror("socket creation failed");
        exit(1);
    }
    
    int one = 1;
    if (setsockopt(raw_sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("setsockopt failed");
        close(raw_sock);
        exit(1);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(raw_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(raw_sock);
        exit(1);
    }
    
    printf("UDP RAW Echo Server running on port %d\n", PORT);
    printf("Waiting for messages...\n");
    
    while (running) {
        memset(buffer, 0, BUFFER_SIZE);
        
        bytes = recvfrom(raw_sock, buffer, BUFFER_SIZE, 0,
                         (struct sockaddr*)&client_addr, &client_len);
        
        if (bytes < 0) {
            if (errno == EINTR) continue;
            perror("recvfrom failed");
            continue;
        }
        
        struct iphdr *ip = (struct iphdr*)buffer;
        int ip_len = ip->ihl * 4;
        
        struct udphdr *udp = (struct udphdr*)(buffer + ip_len);
        char *data = buffer + ip_len + sizeof(struct udphdr);
        int data_len = bytes - ip_len - sizeof(struct udphdr);
        
        if (ntohs(udp->source) == PORT) {
            continue;
        }
        
        if (ntohs(udp->dest) != PORT) {
            continue;
        }
        
        struct sockaddr_in client_sock;
        client_sock.sin_family = AF_INET;
        client_sock.sin_addr.s_addr = ip->saddr;
        client_sock.sin_port = udp->source;
        
        if (data_len >= 5 && strncmp(data, "CLOSE", 5) == 0) {
            remove_client(&client_sock);
            continue;
        }
        
        struct client_info *client = find_client(&client_sock);
        if (!client) {
            printf("Max clients reached, ignoring message\n");
            continue;
        }
        
        printf("Received from %s:%d: %s (message #%d)\n",
               inet_ntoa(client_sock.sin_addr), ntohs(client_sock.sin_port),
               data, client->counter);
        
        snprintf(response, BUFFER_SIZE, "%s %d", data, client->counter);
        client->counter++;
        
        struct sockaddr_in src_addr, dst_addr;
        
        src_addr.sin_family = AF_INET;
        src_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  
        src_addr.sin_port = htons(PORT); 
        
        dst_addr.sin_family = AF_INET;
        dst_addr.sin_addr.s_addr = client_sock.sin_addr.s_addr;
        dst_addr.sin_port = client_sock.sin_port; 
        
        int packet_len = build_udp_packet(packet, &src_addr, &dst_addr,
                                          response, strlen(response));
        
        if (sendto(raw_sock, packet, packet_len, 0,
                   (struct sockaddr*)&dst_addr, sizeof(dst_addr)) < 0) {
            perror("sendto failed");
        }
    }
    
    close(raw_sock);
    printf("Server stopped\n");
    return 0;
}
