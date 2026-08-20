#define _POSIX_C_SOURCE 200809L

#include "common.h"

DriverInfo drivers[MAX_DRIVERS];
int running = 1;

void signal_handler(int signo) {
    (void)signo;
    printf("\nShutting down...\n");
    running = 0;
}

void msleep(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

int send_msg(int fd, const char *msg) {
    return send(fd, msg, strlen(msg), 0) < 0 ? -1 : 0;
}

int recv_msg(int fd, char *buf, size_t bufsz) {
    ssize_t n = recv(fd, buf, bufsz - 1, 0);
    if (n <= 0) return -1;
    buf[n] = '\0';
    return 0;
}

int recv_with_timeout(int fd, char *buf, size_t bufsz, int timeout_ms) {
    struct pollfd fds[1];
    fds[0].fd = fd;
    fds[0].events = POLLIN;
    
    int ret = poll(fds, 1, timeout_ms);
    if (ret <= 0) return -1;
    
    return recv_msg(fd, buf, bufsz);
}

int connect_to_driver(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/tmp/taxi_driver_%d", pid);
    
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%.107s", path);
    
    for (int i = 0; i < 20; i++) {
        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            return sock;
        }
        msleep(50);
    }
    
    close(sock);
    return -1;
}

void create_driver() {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return;
    }
    
    if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);
        
        char path[256];
        snprintf(path, sizeof(path), "/tmp/taxi_driver_%d", getpid());
        
        execl("./driver", "driver", path, NULL);
        perror("execl failed");
        _exit(1);
    }
    
    msleep(500);
    
    for (int i = 0; i < MAX_DRIVERS; i++) {
        if (!drivers[i].active) {
            drivers[i].pid = pid;
            drivers[i].fd = -1;
            drivers[i].active = 1;
            strcpy(drivers[i].status, "Available");
            drivers[i].remaining_time = 0;
            drivers[i].busy_until = 0;
            printf("Driver created with PID: %d\n", pid);
            return;
        }
    }
    
    printf("Maximum drivers reached\n");
}

DriverInfo* find_driver(pid_t pid) {
    for (int i = 0; i < MAX_DRIVERS; i++) {
        if (drivers[i].active && drivers[i].pid == pid) {
            return &drivers[i];
        }
    }
    return NULL;
}

void remove_driver(pid_t pid) {
    for (int i = 0; i < MAX_DRIVERS; i++) {
        if (drivers[i].active && drivers[i].pid == pid) {
            drivers[i].active = 0;
            return;
        }
    }
}

void do_send_task(pid_t pid, int seconds) {
    DriverInfo *d = find_driver(pid);
    if (!d) {
        printf("Driver %d not found\n", pid);
        return;
    }
    
    if (kill(pid, 0) != 0) {
        printf("Driver %d is dead, removing\n", pid);
        remove_driver(pid);
        return;
    }
    
    int sock = connect_to_driver(pid);
    if (sock < 0) {
        printf("Driver %d unavailable\n", pid);
        return;
    }
    
    char req[MSG_MAX];
    snprintf(req, sizeof(req), "TASK %d", seconds);
    
    if (send_msg(sock, req) != 0) {
        printf("Driver %d unavailable\n", pid);
        close(sock);
        return;
    }
    
    char reply[MSG_MAX];
    if (recv_with_timeout(sock, reply, sizeof(reply), REQUEST_TIMEOUT_MS) != 0) {
        printf("Driver %d timeout\n", pid);
        close(sock);
        return;
    }
    
    close(sock);
    
    if (strcmp(reply, "OK") == 0) {
        printf("Task sent to driver %d (%d sec)\n", pid, seconds);
    } else if (strncmp(reply, "BUSY ", 5) == 0) {
        printf("Busy %s\n", reply + 5);
    } else {
        printf("Unexpected reply: %s\n", reply);
    }
}

void do_get_status(pid_t pid) {
    DriverInfo *d = find_driver(pid);
    if (!d) {
        printf("Driver %d not found\n", pid);
        return;
    }
    
    if (kill(pid, 0) != 0) {
        printf("Driver %d is dead, removing\n", pid);
        remove_driver(pid);
        return;
    }
    
    int sock = connect_to_driver(pid);
    if (sock < 0) {
        printf("Driver %d unavailable\n", pid);
        return;
    }
    
    if (send_msg(sock, "STATUS") != 0) {
        printf("Driver %d unavailable\n", pid);
        close(sock);
        return;
    }
    
    char reply[MSG_MAX];
    if (recv_with_timeout(sock, reply, sizeof(reply), REQUEST_TIMEOUT_MS) != 0) {
        printf("Driver %d timeout\n", pid);
        close(sock);
        return;
    }
    
    close(sock);
    
    if (strcmp(reply, "AVAILABLE") == 0) {
        printf("Available\n");
    } else if (strncmp(reply, "BUSY ", 5) == 0) {
        printf("Busy %s\n", reply + 5);
    } else {
        printf("%s\n", reply);
    }
}

void do_get_drivers() {
    int found = 0;
    for (int i = 0; i < MAX_DRIVERS; i++) {
        if (drivers[i].active) {
            printf("  PID: %d", drivers[i].pid);
            
            if (kill(drivers[i].pid, 0) != 0) {
                printf(", Status: dead (removing)\n");
                remove_driver(drivers[i].pid);
                continue;
            }
            
            int sock = connect_to_driver(drivers[i].pid);
            if (sock < 0) {
                printf(", Status: no connection\n");
                continue;
            }
            
            char reply[MSG_MAX];
            if (send_msg(sock, "STATUS") == 0 &&
                recv_with_timeout(sock, reply, sizeof(reply), REQUEST_TIMEOUT_MS) == 0) {
                if (strcmp(reply, "AVAILABLE") == 0) {
                    printf(", Status: Available\n");
                } else if (strncmp(reply, "BUSY ", 5) == 0) {
                    printf(", Status: Busy %s\n", reply + 5);
                } else {
                    printf(", Status: %s\n", reply);
                }
            } else {
                printf(", Status: no response\n");
            }
            close(sock);
            found = 1;
        }
    }
    if (!found) {
        printf("  No drivers\n");
    }
}

void shutdown_all() {
    for (int i = 0; i < MAX_DRIVERS; i++) {
        if (drivers[i].active) {
            int sock = connect_to_driver(drivers[i].pid);
            if (sock >= 0) {
                send_msg(sock, "QUIT");
                close(sock);
            }
            waitpid(drivers[i].pid, NULL, 0);
            drivers[i].active = 0;
        }
    }
}

int main() {
    memset(drivers, 0, sizeof(drivers));
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);
    
    printf("Taxi Dispatcher started\n");
    printf("Commands:\n");
    printf("  create_driver\n");
    printf("  send_task <pid> <task_timer>\n");
    printf("  get_status <pid>\n");
    printf("  get_drivers\n");
    printf("  exit\n");
    
    char line[128];
    while (running) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }
        
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        
        char *cmd = strtok(p, " \t\n");
        if (!cmd) continue;
        
        if (strcmp(cmd, "create_driver") == 0) {
            create_driver();
        } else if (strcmp(cmd, "send_task") == 0) {
            char *a1 = strtok(NULL, " \t\n");
            char *a2 = strtok(NULL, " \t\n");
            if (!a1 || !a2) {
                printf("Usage: send_task <pid> <seconds>\n");
            } else {
                do_send_task((pid_t)atoi(a1), atoi(a2));
            }
        } else if (strcmp(cmd, "get_status") == 0) {
            char *a1 = strtok(NULL, " \t\n");
            if (!a1) {
                printf("Usage: get_status <pid>\n");
            } else {
                do_get_status((pid_t)atoi(a1));
            }
        } else if (strcmp(cmd, "get_drivers") == 0) {
            do_get_drivers();
        } else if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
            printf("Shutting down...\n");
            running = 0;
        } else if (strlen(cmd) > 0) {
            printf("Unknown command: %s\n", cmd);
        }
    }
    
    printf("Stopping all drivers...\n");
    shutdown_all();
    printf("Dispatcher stopped\n");
    return 0;
}
