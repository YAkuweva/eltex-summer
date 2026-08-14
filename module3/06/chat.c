#include "chat.h"
#include "network.h"
#include "utils.h"

char nickname[MAX_NICKNAME];

void handle_exit(int sig) {
    (void)sig;
    send_leave_message();
    printf("\nGoodbye!\n");
    close_socket();
    exit(0);
}

int main(int argc, char **argv) {
    fd_set read_fds;
    struct sockaddr_in sender_addr;
    char send_buffer[BUFFER_SIZE];
    char recv_buffer[BUFFER_SIZE];
    int bytes_received;
    
    if (argc != 2) {
        printf("Usage: %s <your_nickname>\n", argv[0]);
        printf("Example: %s Alice\n", argv[0]);
        exit(1);
    }
    
    if (!is_valid_nickname(argv[1])) {
        printf("Error: Invalid nickname. Use only letters, numbers and '_'\n");
        exit(1);
    }
    
    safe_strcpy(nickname, argv[1], MAX_NICKNAME);
    
    print_network_info();
    
    if (init_socket() < 0) {
        exit(1);
    }
    
    signal(SIGINT, handle_exit);
    
    send_join_message();
    
    printf("   Chat \n");
    printf("Your nickname: %s\n\n", nickname);
    
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(0, &read_fds);      
        FD_SET(sockfd, &read_fds); 
        
        if (select(sockfd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select error");
            break;
        }
        
        if (FD_ISSET(0, &read_fds)) {
            if (read_user_input(send_buffer, BUFFER_SIZE)) {
                if (strcmp(send_buffer, "/quit") == 0) {
                    handle_exit(0);
                }
                else if (strcmp(send_buffer, "/whoami") == 0) {
                    printf("Your nickname: %s\n", nickname);
                }
                else if (strlen(send_buffer) > 0) {
                    send_chat_message(send_buffer);
                }
            }
        }
        
        if (FD_ISSET(sockfd, &read_fds)) {
            bytes_received = receive_message(recv_buffer, BUFFER_SIZE, &sender_addr);
            
            if (bytes_received > 0) {
                if (!is_my_message(recv_buffer)) {
                    display_message(recv_buffer);
                }
            } else if (bytes_received < 0) {
                perror("receive error");
                break;
            }
        }
    }
    
    close_socket();
    return 0;
}
