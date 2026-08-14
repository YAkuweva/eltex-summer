#include "chat.h"

#define FILES_DIR "server_files"
#define MAX_FILE_PATH 512

typedef struct {
    int fd;
    char nickname[MAX_NICKNAME];
    int connected;
} Client;

typedef struct {
    FILE *file;
    char filename[256];
    int bytes_received;
    int total_size;
    int is_receiving;
    int sender_fd;
} FileReceiver;

Client clients[MAX_CLIENTS];
int client_count = 0;
int epoll_fd;
int server_fd_global = -1;  
FileReceiver file_receiver = {NULL, "", 0, 0, 0, -1};

void init_file_storage(void);
void reset_file_receiver(void);
void start_file_receive(const char *filename, int total_size, int sender_fd);
void append_file_data(const char *filename, const char *data, int data_len);
void finish_file_receive(const char *filename);
void list_files_in_storage(char *buffer, int buffer_size);
int send_file_to_client(int client_fd, const char *filename);
void add_client(int fd, const char *nickname);
void remove_client(int fd);
void broadcast_message(Message *msg, int sender_fd);
void handle_client_message(int client_fd);
void handle_new_connection(int server_fd);
int init_server_socket(void);
void cleanup_files(void);
void signal_handler(int sig);


void signal_handler(int sig) {
    (void)sig;
    printf("\n\nShutting down server...\n");
    
    Message msg;
    msg.type = MSG_LEAVE;
    safe_strcpy(msg.nickname, "SERVER", MAX_NICKNAME);
    snprintf(msg.data, sizeof(msg.data), "Server is shutting down");
    broadcast_message(&msg, -1);
    
    cleanup_files();
    
    if (server_fd_global >= 0) {
        close(server_fd_global);
    }
    if (epoll_fd >= 0) {
        close(epoll_fd);
    }
    
    printf("Server stopped. Files cleared.\n");
    exit(0);
}

void cleanup_files(void) {
    DIR *dir;
    struct dirent *entry;
    char filepath[MAX_FILE_PATH];
    
    printf("Cleaning up files\n");
    
    dir = opendir(FILES_DIR);
    if (dir) {
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            snprintf(filepath, sizeof(filepath), "%s/%s", FILES_DIR, entry->d_name);
            if (remove(filepath) == 0) {
                printf("  Removed: %s\n", entry->d_name);
            }
        }
        closedir(dir);
        rmdir(FILES_DIR);
        printf("Removed directory: %s\n", FILES_DIR);
    }
    
    if (file_receiver.file) {
        fclose(file_receiver.file);
        file_receiver.file = NULL;
        file_receiver.is_receiving = 0;
    }
}

void init_file_storage(void) {
    create_directory(FILES_DIR);
    printf("File storage: %s\n", FILES_DIR);
}

void reset_file_receiver(void) {
    if (file_receiver.file) {
        fclose(file_receiver.file);
        file_receiver.file = NULL;
    }
    file_receiver.filename[0] = '\0';
    file_receiver.bytes_received = 0;
    file_receiver.total_size = 0;
    file_receiver.is_receiving = 0;
    file_receiver.sender_fd = -1;
}

void start_file_receive(const char *filename, int total_size, int sender_fd) {
    if (file_receiver.is_receiving && strcmp(file_receiver.filename, filename) != 0) {
        reset_file_receiver();
    }
    
    if (!file_receiver.is_receiving || strcmp(file_receiver.filename, filename) != 0) {
        char filepath[MAX_FILE_PATH];
        snprintf(filepath, sizeof(filepath), "%s/%s", FILES_DIR, filename);
        
        file_receiver.file = fopen(filepath, "wb");
        if (!file_receiver.file) {
            printf("Error: cannot create file %s\n", filepath);
            return;
        }
        
        safe_strcpy(file_receiver.filename, filename, sizeof(file_receiver.filename));
        file_receiver.total_size = total_size;
        file_receiver.bytes_received = 0;
        file_receiver.is_receiving = 1;
        file_receiver.sender_fd = sender_fd;
        
        printf("Receiving: %s (%d bytes)\n", filename, total_size);
    }
}

void append_file_data(const char *filename, const char *data, int data_len) {
    if (!file_receiver.is_receiving || strcmp(file_receiver.filename, filename) != 0) {
        start_file_receive(filename, 0, -1);
    }
    
    if (file_receiver.file) {
        fwrite(data, 1, data_len, file_receiver.file);
        file_receiver.bytes_received += data_len;
        
        printf("\r  %d/%d bytes (%.1f%%)",
               file_receiver.bytes_received,
               file_receiver.total_size,
               (float)file_receiver.bytes_received / file_receiver.total_size * 100);
        fflush(stdout);
        
        if (file_receiver.total_size > 0 &&
            file_receiver.bytes_received >= file_receiver.total_size) {
            fclose(file_receiver.file);
            file_receiver.file = NULL;
            file_receiver.is_receiving = 0;
            printf("\nFile '%s' saved\n", filename);
            
            if (file_receiver.sender_fd >= 0) {
                flush_socket_buffer(file_receiver.sender_fd);
            }
        }
    }
}

void finish_file_receive(const char *filename) {
    if (file_receiver.is_receiving && file_receiver.file) {
        fflush(file_receiver.file);
        fclose(file_receiver.file);
        file_receiver.file = NULL;
        file_receiver.is_receiving = 0;
        printf("\nFile '%s' completed (%d bytes)\n",
               filename, file_receiver.bytes_received);
        
        if (file_receiver.sender_fd >= 0) {
            flush_socket_buffer(file_receiver.sender_fd);
        }
    }
}

void list_files_in_storage(char *buffer, int buffer_size) {
    (void)buffer_size;
    DIR *dir = opendir(FILES_DIR);
    struct dirent *entry;
    struct stat file_stat;
    char filepath[MAX_FILE_PATH];
    char temp_buffer[BUFFER_SIZE];
    
    buffer[0] = '\0';
    strcat(buffer, "   Files on server:\n");
    
    if (!dir) {
        strcat(buffer, "No files available\n");
        return;
    }
    
    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        
        snprintf(filepath, sizeof(filepath), "%s/%s", FILES_DIR, entry->d_name);
        if (stat(filepath, &file_stat) == 0) {
            snprintf(temp_buffer, sizeof(temp_buffer),
                    " %s (%ld bytes)\n", entry->d_name, (long)file_stat.st_size);
            strcat(buffer, temp_buffer);
            count++;
        }
    }
    closedir(dir);
    
    if (count == 0) strcat(buffer, "No files available\n");
    strcat(buffer, " \n");
}

int send_file_to_client(int client_fd, const char *filename) {
    char filepath[MAX_FILE_PATH];
    snprintf(filepath, sizeof(filepath), "%s/%s", FILES_DIR, filename);
    
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        printf("File not found: %s\n", filename);
        return -1;
    }
    
    fseek(file, 0, SEEK_END);
    int total_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    printf("Sending '%s' to client (%d bytes)\n", filename, total_size);
    
    Message msg;
    msg.type = MSG_FILE;
    safe_strcpy(msg.nickname, "SERVER", MAX_NICKNAME);
    safe_strcpy(msg.filename, filename, sizeof(msg.filename));
    msg.total_size = total_size;
    msg.data_len = 0;
    send_all(client_fd, &msg, sizeof(Message));
    
    set_tcp_nodelay(client_fd);
    
    char buffer[BUFFER_SIZE];
    int bytes_read, total_sent = 0;
    
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        Message data_msg;
        data_msg.type = MSG_FILE_DATA;
        safe_strcpy(data_msg.nickname, "SERVER", MAX_NICKNAME);
        safe_strcpy(data_msg.filename, filename, sizeof(data_msg.filename));
        data_msg.data_len = bytes_read;
        data_msg.total_size = total_size;
        memcpy(data_msg.data, buffer, bytes_read);
        
        send_all(client_fd, &data_msg, sizeof(Message));
        total_sent += bytes_read;
        printf("\rProgress: %d/%d bytes", total_sent, total_size);
        fflush(stdout);
    }
    
    printf("\nFile sent\n");
    fclose(file);
    
    msg.type = MSG_FILE_END;
    send_all(client_fd, &msg, sizeof(Message));
    
    flush_socket_buffer(client_fd);
    
    return 0;
}

void add_client(int fd, const char *nickname) {
    if (client_count >= MAX_CLIENTS) {
        printf("Max clients reached\n");
        close(fd);
        return;
    }
    
    clients[client_count].fd = fd;
    safe_strcpy(clients[client_count].nickname, nickname, MAX_NICKNAME);
    clients[client_count].connected = 1;
    client_count++;
    
    printf("[%s] joined (total: %d)\n", nickname, client_count);
    
    Message msg;
    msg.type = MSG_JOIN;
    safe_strcpy(msg.nickname, nickname, MAX_NICKNAME);
    snprintf(msg.data, sizeof(msg.data), "[%s] joined the chat", nickname);
    broadcast_message(&msg, fd);
}

void remove_client(int fd) {
    for (int i = 0; i < client_count; i++) {
        if (clients[i].fd == fd) {
            char nickname[MAX_NICKNAME];
            safe_strcpy(nickname, clients[i].nickname, MAX_NICKNAME);
            
            Message msg;
            msg.type = MSG_LEAVE;
            safe_strcpy(msg.nickname, nickname, MAX_NICKNAME);
            snprintf(msg.data, sizeof(msg.data), "[%s] left the chat", nickname);
            
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
            close(fd);
            
            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--;
            
            broadcast_message(&msg, -1);
            printf("[%s] left (total: %d)\n", nickname, client_count);
            return;
        }
    }
}

void broadcast_message(Message *msg, int sender_fd) {
    for (int i = 0; i < client_count; i++) {
        if (clients[i].fd != sender_fd) {
            if (send_all(clients[i].fd, msg, sizeof(Message)) == -1) {
                remove_client(clients[i].fd);
                i--;
            }
        }
    }
}

void handle_client_message(int client_fd) {
    Message msg;
    ssize_t bytes;
    
    bytes = recv(client_fd, &msg, sizeof(Message), 0);
    
    if (bytes <= 0) {
        remove_client(client_fd);
        return;
    }
    
    if (bytes < (ssize_t)sizeof(Message)) {
        printf("Warning: incomplete message received (%zd bytes)\n", bytes);
        return;
    }
    
    switch (msg.type) {
        case MSG_JOIN:
            add_client(client_fd, msg.nickname);
            break;
            
        case MSG_CHAT:
            printf("[%s]: %s\n", msg.nickname, msg.data);
            broadcast_message(&msg, client_fd);
            break;
            
        case MSG_FILE:
            printf("[%s] sending: %s (%d bytes)\n",
                   msg.nickname, msg.filename, msg.total_size);
            start_file_receive(msg.filename, msg.total_size, client_fd);
            if (msg.data_len > 0) append_file_data(msg.filename, msg.data, msg.data_len);
            broadcast_message(&msg, client_fd);
            break;
            
        case MSG_FILE_DATA:
            if (!file_receiver.is_receiving ||
                strcmp(file_receiver.filename, msg.filename) != 0) {
                start_file_receive(msg.filename, msg.total_size, client_fd);
            }
            append_file_data(msg.filename, msg.data, msg.data_len);
            broadcast_message(&msg, client_fd);
            break;
            
        case MSG_FILE_END:
            printf("[%s] file sent: %s\n", msg.nickname, msg.filename);
            finish_file_receive(msg.filename);
            broadcast_message(&msg, client_fd);
            break;
            
        case MSG_LEAVE:
            remove_client(client_fd);
            break;
            
        case MSG_FILE_LIST: {
            Message response;
            response.type = MSG_FILE_INFO;
            list_files_in_storage(response.data, sizeof(response.data));
            send_all(client_fd, &response, sizeof(Message));
            break;
        }
            
        case MSG_FILE_GET:
            send_file_to_client(client_fd, msg.filename);
            break;
            
        default:
            printf("Unknown type: %d\n", msg.type);
    }
}

int init_server_socket(void) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket creation");
        return -1;
    }
    
    int reuse = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_fd);
        return -1;
    }
    
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        close(server_fd);
        return -1;
    }
    
    set_nonblocking(server_fd);
    printf("Listening on port %d\n", PORT);
    return server_fd;
}

void handle_new_connection(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("accept");
        }
        return;
    }
    
    printf("New connection from %s:%d\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    set_nonblocking(client_fd);
    set_tcp_nodelay(client_fd); 
    
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
        perror("epoll_ctl");
        close(client_fd);
    }
}

int main(void) {
    printf("   TCP Chat Server \n");
    printf("Port: %d\n", PORT);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    init_file_storage();
    
    server_fd_global = init_server_socket();
    if (server_fd_global < 0) exit(1);
    
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        close(server_fd_global);
        exit(1);
    }
    
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = server_fd_global;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd_global, &ev);
    
    struct epoll_event events[MAX_EVENTS];
    
    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }
        
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server_fd_global) {
                handle_new_connection(server_fd_global);
            } else {
                handle_client_message(events[i].data.fd);
            }
        }
    }
    
    close(server_fd_global);
    close(epoll_fd);
    return 0;
}
