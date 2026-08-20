#define _POSIX_C_SOURCE 200809L

#include "common.h"

int server_sock;
int running = 1;
char status[BUFFER_SIZE];
time_t busy_until = 0;

void signal_handler(int signo) {
    (void)signo;
    running = 0;
    close(server_sock);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <socket_path>\n", argv[0]);
        exit(1);
    }
    
    char socket_path[256];
    strcpy(socket_path, argv[1]);
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    if ((server_sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        exit(1);
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%.107s", socket_path);
    
    unlink(socket_path);
    
    if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        close(server_sock);
        exit(1);
    }
    
    if (listen(server_sock, 1) < 0) {
        perror("listen failed");
        close(server_sock);
        exit(1);
    }
    
    strcpy(status, "Available");
    busy_until = 0;
    
    printf("Driver started (PID: %d)\n", getpid());
    fflush(stdout);
    
    while (running) {
        struct pollfd fds[1];
        fds[0].fd = server_sock;
        fds[0].events = POLLIN;
        
        int ret = poll(fds, 1, 1000);
        
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("poll failed");
            break;
        }
        
        if (busy_until > 0 && time(NULL) >= busy_until) {
            busy_until = 0;
            strcpy(status, "Available");
            // printf("[DRIVER] Task completed, status: Available\n");
            fflush(stdout);
        }
        
        if (ret == 0) {
            continue;
        }
        
        if (fds[0].revents & POLLIN) {
            int client_fd = accept(server_sock, NULL, NULL);
            if (client_fd < 0) {
                if (errno == EINTR) continue;
                perror("accept failed");
                continue;
            }
            
            char buf[MSG_MAX];
            char response[MSG_MAX];
            int bytes = recv(client_fd, buf, sizeof(buf) - 1, 0);
            
            if (bytes <= 0) {
                close(client_fd);
                continue;
            }
            buf[bytes] = '\0';
            
            char *p = strchr(buf, '\n');
            if (p) *p = '\0';
            
            response[0] = '\0';
            
            if (strcmp(buf, "STATUS") == 0) {
                if (busy_until > 0 && time(NULL) < busy_until) {
                    int left = (int)(busy_until - time(NULL));
                    if (left < 0) left = 0;
                    snprintf(response, sizeof(response), "BUSY %d", left);
                } else {
                    strcpy(response, "AVAILABLE");
                }
            } else if (strncmp(buf, "TASK ", 5) == 0) {
                int seconds = atoi(buf + 5);
                
                if (busy_until > 0 && time(NULL) < busy_until) {
                    int left = (int)(busy_until - time(NULL));
                    if (left < 0) left = 0;
                    snprintf(response, sizeof(response), "BUSY %d", left);
                } else if (seconds <= 0) {
                    strcpy(response, "OK");
                } else {
                    busy_until = time(NULL) + seconds;
                    strcpy(status, "Busy");
                    strcpy(response, "OK");
                }
            } else if (strcmp(buf, "QUIT") == 0) {
                strcpy(response, "OK");
                send(client_fd, response, strlen(response), 0);
                close(client_fd);
                running = 0;
                break;
            } else {
                strcpy(response, "UNKNOWN");
            }
            
            if (response[0] != '\0') {
                send(client_fd, response, strlen(response), 0);
            }
            
            close(client_fd);
        }
    }
    
    close(server_sock);
    unlink(socket_path);
    printf("Driver stopped\n");
    return 0;
}
