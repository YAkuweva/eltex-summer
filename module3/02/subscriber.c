#include "broker.h"

int subscriber_msqid;
pid_t subscriber_pid;

void subscriber_signal_handler(int sig) {
    printf("\nSubscriber: Exiting...\n");
    exit(0);
}

void run_subscriber(char *topics_str) {
    subscriber_pid = getpid();
    
    key_t key = ftok("/tmp", 'M');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }
    
    subscriber_msqid = msgget(key, 0666);
    if (subscriber_msqid == -1) {
        perror("msgget");
        printf("No broker running. Start broker first (./broker_system -b)\n");
        exit(1);
    }
    
    signal(SIGINT, subscriber_signal_handler);
    signal(SIGTERM, subscriber_signal_handler);
    
    printf("Subscriber (PID: %d)\n", subscriber_pid);
    
    char topics[10][64];
    int topic_count = 0;
    char *token = strtok(topics_str, ",");
    while (token != NULL && topic_count < 10) {
        strcpy(topics[topic_count], token);
        topic_count++;
        token = strtok(NULL, ",");
    }
    
    struct msgbuf sub_msg;
    sub_msg.mtype = 1;
    for (int i = 0; i < topic_count; i++) {
        snprintf(sub_msg.mtext, sizeof(sub_msg.mtext), 
                "subscribe,%d,%s", subscriber_pid, topics[i]);
        if (msgsnd(subscriber_msqid, &sub_msg, strlen(sub_msg.mtext) + 1, 0) == -1) {
            perror("msgsnd");
            exit(1);
        }
        printf("Subscribed to '%s'\n", topics[i]);
    }
    
    printf("Waiting for messages...\n");
    
    struct msgbuf rcvd_msg;
    while (1) {
        if (msgrcv(subscriber_msqid, &rcvd_msg, sizeof(rcvd_msg.mtext), subscriber_pid, 0) == -1) {
            if (errno == EINTR) continue;
            if (errno == EIDRM) {
                printf("Broker exited.\n");
                break;
            }
            perror("msgrcv");
            break;
        }
        
        char topic[64];
        char payload[256];
        parse_received_message(rcvd_msg.mtext, topic, payload);
        printf("Received on '%s': %s\n", topic, payload);
    }
    
    for (int i = 0; i < topic_count; i++) {
        snprintf(sub_msg.mtext, sizeof(sub_msg.mtext), 
                "unsubscribe,%d,%s", subscriber_pid, topics[i]);
        msgsnd(subscriber_msqid, &sub_msg, strlen(sub_msg.mtext) + 1, 0);
        printf("Unsubscribed from '%s'\n", topics[i]);
    }
}
