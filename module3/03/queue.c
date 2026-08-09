#include "chat.h"

char queue_name_1[64];
char queue_name_2[64];
mqd_t queue_send = -1;
mqd_t queue_recv = -1;
bool is_creator = false;

int init_queues(const char *base_name) {
    snprintf(queue_name_1, sizeof(queue_name_1), "/%s_1", base_name);
    snprintf(queue_name_2, sizeof(queue_name_2), "/%s_2", base_name);
    
    printf("Queue 1: %s\n", queue_name_1);
    printf("Queue 2: %s\n", queue_name_2);
    
    struct mq_attr attr;
    attr.mq_maxmsg = MAX_MSG_COUNT;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_flags = 0;
    
    mqd_t mq1 = mq_open(queue_name_1, O_CREAT | O_RDWR | O_EXCL, 0666, &attr);
    
    if (mq1 != (mqd_t)-1) {
        is_creator = true;
        printf("Created new queues. Receiving via %s, sending via %s\n", 
               queue_name_1, queue_name_2);
        
        mqd_t mq2 = mq_open(queue_name_2, O_CREAT | O_RDWR, 0666, &attr);
        if (mq2 == (mqd_t)-1) {
            perror("Failed to create second queue");
            mq_unlink(queue_name_1);
            return -1;
        }
        
        queue_recv = mq1;
        queue_send = mq2;
    } else {
        printf("Queues already exist. Sending via %s, receiving via %s\n",
               queue_name_1, queue_name_2);
        
        mqd_t mq1_existing = mq_open(queue_name_1, O_RDWR);
        if (mq1_existing == (mqd_t)-1) {
            perror("Failed to open existing queue 1");
            return -1;
        }
        
        mqd_t mq2_existing = mq_open(queue_name_2, O_RDWR);
        if (mq2_existing == (mqd_t)-1) {
            perror("Failed to open existing queue 2");
            mq_close(mq1_existing);
            return -1;
        }
        
        queue_send = mq1_existing;
        queue_recv = mq2_existing;
    }
    
    return 0;
}

void cleanup_queues(void) {
    if (!is_creator) {
        return;
    }
    
    if (queue_recv != -1) {
        mq_close(queue_recv);
    }
    if (queue_send != -1) {
        mq_close(queue_send);
    }
    
    if (mq_unlink(queue_name_1) == -1) {
        perror("Failed to unlink queue 1");
    } else {
        printf("Queue %s removed\n", queue_name_1);
    }
    
    if (mq_unlink(queue_name_2) == -1) {
        perror("Failed to unlink queue 2");
    } else {
        printf("Queue %s removed\n", queue_name_2);
    }
}

void send_exit_message(void) {
    if (queue_send != -1) {
        if (mq_send(queue_send, EXIT_MSG, strlen(EXIT_MSG) + 1, EXIT_PRIORITY) == -1) {
            perror("Failed to send exit message");
        } else {
            printf("Exit message sent\n");
        }
    }
}

int receive_message(char *buffer, unsigned int *priority) {
    if (queue_recv == -1) {
        return -1;
    }
    
    ssize_t bytes = mq_receive(queue_recv, buffer, MAX_MSG_SIZE, priority);
    if (bytes == -1) {
        if (errno != EAGAIN) {
            perror("Failed to receive message");
        }
        return -1;
    }
    
    return (int)bytes;
}

int send_message(const char *message, unsigned int priority) {
    if (queue_send == -1) {
        return -1;
    }
    
    if (mq_send(queue_send, message, strlen(message) + 1, priority) == -1) {
        perror("Failed to send message");
        return -1;
    }
    
    return 0;
}

int check_messages_available(void) {
    if (queue_recv == -1) {
        return -1;
    }
    
    struct mq_attr attr;
    if (mq_getattr(queue_recv, &attr) == -1) {
        perror("Failed to get queue attributes");
        return -1;
    }
    
    return (int)attr.mq_curmsgs;
}

int check_keyboard_input(void) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(0, &read_fds);
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = TIMEOUT_USEC;
    
    return select(1, &read_fds, NULL, NULL, &tv);
}

int read_user_input(char *buffer, size_t size) {
    if (fgets(buffer, size, stdin) == NULL) {
        return 0;
    }
    
     size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
    }
    
    return (len > 0) ? 1 : 0;
}
