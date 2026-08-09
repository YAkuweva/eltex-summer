#include "chat.h"

void print_chat_started(void) {
    printf("\n   Chat start!!    \n");
    printf("\n\n");
}

int handle_received_message(char *buffer, unsigned int priority) {
    if (priority == EXIT_PRIORITY) {
        printf("\n[Peer exited]\n");
        return 1;  
    }
    
    printf("[Peer]: %s\n", buffer);
    fflush(stdout);
    return 0;
}

int main(int argc, char *argv[]) {
    
    if (setup_signal_handler() == -1) {
        return 1;
    }
    
    if (init_queues(argv[1]) == -1) {
        return 1;
    }
    
    print_chat_started();
    
    char send_buf[MAX_MSG_SIZE];
    char recv_buf[MAX_MSG_SIZE];
    unsigned int priority;
    bool should_exit = false;
    
    while (!should_exit) {
        int msg_count = check_messages_available();
        if (msg_count < 0) {
            break;
        }
        
        while (msg_count > 0 && !should_exit) {
            if (receive_message(recv_buf, &priority) > 0) {
                if (handle_received_message(recv_buf, priority)) {
                    should_exit = true;
                }
            }
            msg_count = check_messages_available();
        }
        
        if (check_keyboard_input()) {
            if (read_user_input(send_buf, sizeof(send_buf))) {
                if (strcmp(send_buf, "exit") == 0 || strcmp(send_buf, "quit") == 0) {
                    send_exit_message();
                    should_exit = true;
                    break;
                }
                
                if (strlen(send_buf) > 0) {
                    send_message(send_buf, NORMAL_PRIORITY);
                }
            }
        }
    }
    
    if (is_creator) {
        cleanup_queues();
    }
    
    printf("Chat finished\n");
    return 0;
}
