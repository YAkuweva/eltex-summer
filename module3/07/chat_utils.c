#include "chat.h"

void safe_strcpy(char *dest, const char *src, size_t size) {
    if (dest == NULL || src == NULL || size == 0) return;
    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
}

int send_all(int fd, const void *buf, size_t len) {
    size_t total_sent = 0;
    const char *ptr = (const char *)buf;
    
    while (total_sent < len) {
        ssize_t sent = send(fd, ptr + total_sent, len - total_sent, 0);
        if (sent <= 0) {
            if (sent == -1 && errno == EAGAIN) continue;
            return -1;
        }
        total_sent += sent;
    }
    return 0;
}

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
    }
}

void set_tcp_nodelay(int fd) {
    int enable = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable)) == -1) {
        perror("setsockopt TCP_NODELAY");
    }
}

void flush_socket_buffer(int fd) {
    int enable = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
    usleep(1000);
    enable = 0;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
}

int create_directory(const char *dir_name) {
    struct stat st = {0};
    if (stat(dir_name, &st) == -1) {
        if (mkdir(dir_name, 0755) == -1) {
            perror("mkdir failed");
            return -1;
        }
        return 1; 
    }
    return 0;
}

long get_file_size(FILE *file) {
    long current = ftell(file);
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, current, SEEK_SET);
    return size;
}

int write_file_chunk(FILE *file, const char *data, int data_len) {
    size_t written = fwrite(data, 1, data_len, file);
    return (written == (size_t)data_len) ? 0 : -1;
}
