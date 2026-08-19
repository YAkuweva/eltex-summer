#include "common.h"

int raw_sock;
int running = 1;
struct sockaddr_in server_addr, client_addr;

void signal_handler(int signo) {
    (void)signo;
    printf("\nSending close message to server...\n");
    
    char close_msg[] = "CLOSE";
    char packet[4096];
    struct iphdr *ip = (struct iphdr*)packet;
    struct udphdr *udp = (struct udphdr*)(packet + sizeof(struct iphdr));
    char *data = packet + sizeof(struct iphdr) + sizeof(struct udphdr);
    
    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + 5);
    ip->id = htons(rand() % 65535);
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = IPPROTO_UDP;
    ip->check = 0;
    ip->saddr = client_addr.sin_addr.s_addr;
    ip->daddr = server_addr.sin_addr.s_addr;
    
    udp->source = client_addr.sin_port;
    udp->dest = server_addr.sin_port;
    udp->len = htons(sizeof(struct udphdr) + 5);
    udp->check = 0;
    
    memcpy(data, close_msg, 5);
    udp->check = udp_checksum(ip, udp, close_msg, 5);
    
    sendto(raw_sock, packet, sizeof(struct iphdr) + sizeof(struct udphdr) + 5, 0,
           (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    running = 0;
    close(raw_sock);
    printf("Client shut down\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char packet[4096];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    int bytes;
    fd_set read_fds;
    int client_port;
    
    if (argc != 3) {
        printf("Usage: %s <server_ip> <client_port>\n", argv[0]);
        printf("Example: %s 127.0.0.1 12345\n", argv[0]);
        exit(1);
    }
    
    client_port = atoi(argv[2]);
    if (client_port < 1024 || client_port > 65535) {
        printf("Port must be between 1024 and 65535\n");
        exit(1);
    }
    
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
    
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = htons(client_port);
    
    if (bind(raw_sock, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
        perror("bind failed");
        close(raw_sock);
        exit(1);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_aton(argv[1], &server_addr.sin_addr) == 0) {
        printf("Invalid IP address\n");
        close(raw_sock);
        exit(1);
    }
    
    printf("UDP RAW Echo Client\n");
    printf("Client port: %d\n", client_port);
    printf("Server: %s:%d\n", argv[1], PORT);
    
    while (running) {
        FD_ZERO(&read_fds);
        FD_SET(0, &read_fds);
        FD_SET(raw_sock, &read_fds);
        
        if (select(raw_sock + 1, &read_fds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            perror("select failed");
            break;
        }
        
        if (FD_ISSET(0, &read_fds)) {
            if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
                continue;
            }
            buffer[strcspn(buffer, "\n")] = 0;
            
            if (strlen(buffer) == 0) continue;
            
            struct iphdr *ip = (struct iphdr*)packet;
            struct udphdr *udp = (struct udphdr*)(packet + sizeof(struct iphdr));
            char *data = packet + sizeof(struct iphdr) + sizeof(struct udphdr);
            int data_len = strlen(buffer);
            
            ip->version = 4;
            ip->ihl = 5;
            ip->tos = 0;
            ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + data_len);
            ip->id = htons(rand() % 65535);
            ip->frag_off = 0;
            ip->ttl = 64;
            ip->protocol = IPPROTO_UDP;
            ip->check = 0;
            ip->saddr = client_addr.sin_addr.s_addr;
            ip->daddr = server_addr.sin_addr.s_addr;
            
            udp->source = client_addr.sin_port;
            udp->dest = server_addr.sin_port;
            udp->len = htons(sizeof(struct udphdr) + data_len);
            udp->check = 0;
            
            memcpy(data, buffer, data_len);
            udp->check = udp_checksum(ip, udp, buffer, data_len);
            
            int packet_len = sizeof(struct iphdr) + sizeof(struct udphdr) + data_len;
            
            if (sendto(raw_sock, packet, packet_len, 0,
                       (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                perror("sendto failed");
                continue;
            }
            
        }
        
        if (FD_ISSET(raw_sock, &read_fds)) {
            memset(response, 0, BUFFER_SIZE);
            bytes = recvfrom(raw_sock, response, BUFFER_SIZE, 0,
                             (struct sockaddr*)&from_addr, &from_len);
            
            if (bytes < 0) {
                if (errno == EINTR) continue;
                perror("recvfrom failed");
                continue;
            }
            
            struct iphdr *ip = (struct iphdr*)response;
            int ip_len = ip->ihl * 4;
            struct udphdr *udp = (struct udphdr*)(response + ip_len);
            char *data = response + ip_len + sizeof(struct udphdr);
            int data_len = bytes - ip_len - sizeof(struct udphdr);
            
            if (ntohs(udp->source) == client_port) {
                continue;
            }
            
            if (ntohs(udp->source) != PORT) {
                continue;
            }
            
            if (ntohs(udp->dest) != client_port) {
                continue;
            }
            
            if (ip->saddr != server_addr.sin_addr.s_addr) {
                continue;
            }
            
            if (data_len > 0) {
                data[data_len] = 0;
                printf("Server response: %s\n", data);
            }
        }
    }
    
    close(raw_sock);
    return 0;
}
