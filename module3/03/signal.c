#include "chat.h"

extern void cleanup_queues(void);
extern void send_exit_message(void);

void signal_handler(int sig) {
    printf("\nReceived SIGINT. Finish \n");
    
    send_exit_message();
    
    cleanup_queues();
    
    exit(0);
}

int setup_signal_handler(void) {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Failed to set signal handler");
        return -1;
    }
    
    return 0;
}
