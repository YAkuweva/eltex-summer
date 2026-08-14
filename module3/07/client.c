#include "chat.h"

#define RECEIVED_DIR "received_files"

int sockfd;
char nickname[MAX_NICKNAME];

void connect_to_server(const char *server_ip);
void send_message(int type, const char *data);
void send_file(const char *filename);
void receive_file(Message *msg);
void handle_server_message(void);
void handle_user_input(void);

void connect_to_server(const char *server_ip) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket creation");
        exit(1);
    }
    
    set_tcp_nodelay(sockfd);
    
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    if (inet_aton(server_ip, &server_addr.sin_addr) == 0) {
        fprintf(stderr, "Invalid IP: %s\n", server_ip);
        close(sockfd);
        exit(1);
    }
    
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connection failed");
        close(sockfd);
        exit(1);
    }
    
    printf("Connected to %s:%d\n", server_ip, PORT);
}

void send_message(int type, const char *data) {
    Message msg;
    msg.type = type;
    safe_strcpy(msg.nickname, nickname, MAX_NICKNAME);
    safe_strcpy(msg.data, data, BUFFER_SIZE);
    msg.data_len = strlen(data);
    msg.total_size = msg.data_len;
    
    if (send_all(sockfd, &msg, sizeof(Message)) == -1) {
        perror("send error");
        close(sockfd);
        exit(1);
    }
    
    flush_socket_buffer(sockfd);
}

void send_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Cannot open file: %s\n", filename);
        return;
    }
    
    fseek(file, 0, SEEK_END);
    int total_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    printf("Sending: %s (%d bytes)\n", filename, total_size);
    
    Message header;
    header.type = MSG_FILE;
    safe_strcpy(header.nickname, nickname, MAX_NICKNAME);
    safe_strcpy(header.filename, filename, sizeof(header.filename));
    header.total_size = total_size;
    header.data_len = 0;
    send_all(sockfd, &header, sizeof(Message));
    flush_socket_buffer(sockfd);
    
    char buffer[BUFFER_SIZE];
    int bytes_read, total_sent = 0;
    
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        Message data_msg;
        data_msg.type = MSG_FILE_DATA;
        safe_strcpy(data_msg.nickname, nickname, MAX_NICKNAME);
        safe_strcpy(data_msg.filename, filename, sizeof(data_msg.filename));
        data_msg.data_len = bytes_read;
        data_msg.total_size = total_size;
        memcpy(data_msg.data, buffer, bytes_read);
        
        if (send_all(sockfd, &data_msg, sizeof(Message)) == -1) {
            perror("send chunk error");
            break;
        }
        
        flush_socket_buffer(sockfd);
        
        total_sent += bytes_read;
        printf("\rProgress: %d/%d (%.1f%%)", total_sent, total_size,
               (float)total_sent / total_size * 100);
        fflush(stdout);
    }
    
    printf("\nFile sent!\n");
    fclose(file);
    
    Message end;
    end.type = MSG_FILE_END;
    safe_strcpy(end.nickname, nickname, MAX_NICKNAME);
    safe_strcpy(end.filename, filename, sizeof(end.filename));
    end.total_size = total_size;
    end.data_len = 0;
    send_all(sockfd, &end, sizeof(Message));
    flush_socket_buffer(sockfd);
}

void receive_file(Message *msg) {
    static char current_file[256] = "";
    static FILE *file = NULL;
    static int bytes_received = 0;
    static int total_size = 0;
    
    if (msg->type == MSG_FILE) {
        if (file) {
            fclose(file);
            file = NULL;
        }
        
        safe_strcpy(current_file, msg->filename, sizeof(current_file));
        total_size = msg->total_size;
        bytes_received = 0;
        
        create_directory(RECEIVED_DIR);
        
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", RECEIVED_DIR, current_file);
        
        file = fopen(filepath, "wb");
        if (!file) {
            printf("Cannot create file: %s\n", filepath);
            return;
        }
        
        printf("\n[%s] sending file: %s (%d bytes)\n",
               msg->nickname, current_file, total_size);
        printf("Saving to: %s\n", filepath);
        
        if (msg->data_len > 0) {
            fwrite(msg->data, 1, msg->data_len, file);
            bytes_received += msg->data_len;
        }
    }
    else if (msg->type == MSG_FILE_DATA) {
        if (!file) {
            create_directory(RECEIVED_DIR);
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", RECEIVED_DIR, msg->filename);
            safe_strcpy(current_file, msg->filename, sizeof(current_file));
            total_size = msg->total_size;
            bytes_received = 0;
            file = fopen(filepath, "wb");
            if (!file) return;
        }
        
        if (strcmp(current_file, msg->filename) != 0) {
            if (file) {
                fclose(file);
                file = NULL;
            }
            safe_strcpy(current_file, msg->filename, sizeof(current_file));
            total_size = msg->total_size;
            bytes_received = 0;
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", RECEIVED_DIR, current_file);
            file = fopen(filepath, "wb");
            if (!file) return;
        }
        
        if (file) {
            fwrite(msg->data, 1, msg->data_len, file);
            bytes_received += msg->data_len;
            printf("\rReceiving: %d/%d (%.1f%%)", bytes_received, total_size,
                   (float)bytes_received / total_size * 100);
            fflush(stdout);
            
            if (bytes_received >= total_size) {
                fclose(file);
                file = NULL;
                printf("\nFile '%s' received!\n", current_file);
            }
        }
    }
    else if (msg->type == MSG_FILE_END) {
        if (file) {
            fclose(file);
            file = NULL;
            printf("\nFile '%s' received!\n", current_file);
        }
        printf("[%s] finished sending\n", msg->nickname);
    }
}

void handle_server_message(void) {
    Message msg;
    ssize_t bytes;
    
    bytes = recv(sockfd, &msg, sizeof(Message), 0);
    
    if (bytes <= 0) {
        printf("Server disconnected\n");
        close(sockfd);
        exit(1);
    }
    
    if (bytes < (ssize_t)sizeof(Message)) {
        printf("Warning: incomplete message received (%zd bytes)\n", bytes);
        return;
    }
    
    switch (msg.type) {
        case MSG_JOIN:
        case MSG_LEAVE:
            printf("%s\n", msg.data);
            break;
        case MSG_CHAT:
            if (strcmp(msg.nickname, nickname) != 0) {
                printf("[%s]: %s\n", msg.nickname, msg.data);
            }
            break;
        case MSG_FILE:
        case MSG_FILE_DATA:
        case MSG_FILE_END:
            receive_file(&msg);
            break;
        case MSG_FILE_INFO:
            printf("\n%s\n", msg.data);
            break;
        default:
            printf("Unknown message type: %d\n", msg.type);
    }
    fflush(stdout);
}

void handle_user_input(void) {
    char input[BUFFER_SIZE];
    if (fgets(input, sizeof(input), stdin) == NULL) return;
    
    input[strcspn(input, "\n")] = '\0';
    if (strlen(input) == 0) return;
    
    if (input[0] == '/') {
        if (strncmp(input, "/file ", 6) == 0) {
            send_file(input + 6);
        }
        else if (strcmp(input, "/list") == 0) {
            Message msg;
            msg.type = MSG_FILE_LIST;
            safe_strcpy(msg.nickname, nickname, MAX_NICKNAME);
            send_all(sockfd, &msg, sizeof(Message));
            flush_socket_buffer(sockfd);
        }
        else if (strncmp(input, "/get ", 5) == 0) {
            Message msg;
            msg.type = MSG_FILE_GET;
            safe_strcpy(msg.nickname, nickname, MAX_NICKNAME);
            safe_strcpy(msg.filename, input + 5, sizeof(msg.filename));
            send_all(sockfd, &msg, sizeof(Message));
            flush_socket_buffer(sockfd);
        }
        else {
            printf("Unknown command. Type /help\n");
        }
    } else {

        send_message(MSG_CHAT, input);
        
        printf("[%s]: %s\n", nickname, input);
        fflush(stdout);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Example: %s 127.0.0.1 Alice\n", argv[0]);
        exit(1);
    }
    
    safe_strcpy(nickname, argv[2], MAX_NICKNAME);
    
    printf("   TCP Chat Client \n");
    printf("Server: %s:%d\n", argv[1], PORT);
    printf("Nickname: %s\n", nickname);
    printf("Files: %s/\n", RECEIVED_DIR);
        
    create_directory(RECEIVED_DIR);
    connect_to_server(argv[1]);
    
    Message join_msg;
    join_msg.type = MSG_JOIN;
    safe_strcpy(join_msg.nickname, nickname, MAX_NICKNAME);
    join_msg.data_len = 0;
    send_all(sockfd, &join_msg, sizeof(Message));
    flush_socket_buffer(sockfd);
    
    fd_set read_fds;
    
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(0, &read_fds);
        FD_SET(sockfd, &read_fds);
        
        int max_fd = (sockfd > 0) ? sockfd : 0;
        
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }
        
        if (FD_ISSET(0, &read_fds)) handle_user_input();
        if (FD_ISSET(sockfd, &read_fds)) handle_server_message();
    }
    
    close(sockfd);
    return 0;
}
