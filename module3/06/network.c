#include "network.h"

int sockfd;
struct sockaddr_in broadcast_addr;

int init_socket(void) {
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation error");
        return -1;
    }

    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt SO_REUSEADDR error");
        close(sockfd);
        return -1;
    }
    int broadcast_enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, 
                   &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("setsockopt error");
        close(sockfd);
        return -1;
    }
    
    struct sockaddr_in my_addr;
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(PORT);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        perror("bind error");
        close(sockfd);
        return -1;
    }
    
    bzero(&broadcast_addr, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(PORT);
    broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    
    printf("Broadcasting to: %s:%d\n", 
           inet_ntoa(broadcast_addr.sin_addr), 
           ntohs(broadcast_addr.sin_port));
    return sockfd;
}

int send_broadcast(const char *message) {
    int result = sendto(sockfd, message, strlen(message), 0,
                        (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr));
    if (result < 0) {
        perror("sendto error");
    }
    return result;
}

int receive_message(char *buffer, int buffer_size, struct sockaddr_in *sender_addr) {
    socklen_t sender_len = sizeof(*sender_addr);
    int bytes = recvfrom(sockfd, buffer, buffer_size - 1, 0,
                         (struct sockaddr *)sender_addr, &sender_len);
    if (bytes > 0) {
        buffer[bytes] = '\0';
    }
    return bytes;
}

void send_join_message(void) {
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "[%s] joined the chat", nickname);
    send_broadcast(message);
}

void send_leave_message(void) {
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "[%s] left the chat", nickname);
    send_broadcast(message);
}

void send_chat_message(const char *text) {
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "[%s]: %s", nickname, text);
    send_broadcast(message);
}

void print_network_info(void) {
    printf("Network: \n");
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        printf("hostname: %s\n", hostname);
        
        struct hostent *host = gethostbyname(hostname);
        if (host) {
            int i = 0;
            while (host->h_addr_list[i] != NULL) {
                struct in_addr addr;
                memcpy(&addr, host->h_addr_list[i], sizeof(addr));
                printf("IP address %d: %s\n", i+1, inet_ntoa(addr));
                i++;
            }
        }
    }
}

void close_socket(void) {
    if (sockfd >= 0) {
        close(sockfd);
    }
}
