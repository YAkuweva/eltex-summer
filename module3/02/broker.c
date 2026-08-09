#include "broker.h"

int msqid = -1;
struct subscription subscriptions[MAX_SUBSCRIPTIONS];
int sub_count = 0;
struct publisher publishers[MAX_PID_LIST];
int pub_count = 0;

void find_subscribers(char *topic, pid_t *found_pids, int *found_count) {
    *found_count = 0;
    for (int i = 0; i < sub_count; i++) {
        if (strcmp(subscriptions[i].topic, topic) == 0) {
            found_pids[*found_count] = subscriptions[i].pid;
            (*found_count)++;
        }
    }
}

void remove_subscription(pid_t pid, char *topic) {
    for (int i = 0; i < sub_count; i++) {
        if (subscriptions[i].pid == pid && strcmp(subscriptions[i].topic, topic) == 0) {
            for (int j = i; j < sub_count - 1; j++) {
                subscriptions[j] = subscriptions[j + 1];
            }
            sub_count--;
            return;
        }
    }
}

void remove_publisher(pid_t pid) {
    for (int i = 0; i < pub_count; i++) {
        if (publishers[i].pid == pid) {
            for (int j = i; j < pub_count - 1; j++) {
                publishers[j] = publishers[j + 1];
            }
            pub_count--;
            return;
        }
    }
}

void broker_signal_handler(int sig) {
    printf("\nBroker: Shutting down...\n");
    
    for (int i = 0; i < pub_count; i++) {
        printf("Broker: Sending SIGINT to publisher %d\n", publishers[i].pid);
        kill(publishers[i].pid, SIGINT);
    }
    
    for (int i = 0; i < sub_count; i++) {
        printf("Broker: Sending SIGINT to subscriber %d\n", subscriptions[i].pid);
        kill(subscriptions[i].pid, SIGINT);
    }
    
    if (msqid != -1) {
        msgctl(msqid, IPC_RMID, NULL);
        printf("Broker: Message queue removed\n");
    }
    exit(0);
}

void run_broker(void) {
    key_t key = ftok("/tmp", 'M');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }
    
    msqid = msgget(key, IPC_CREAT | IPC_EXCL | 0666);
    if (msqid == -1) {
        if (errno == EEXIST) {
            printf("Broker: Another broker is already running.\n");
            exit(1);
        }
        perror("msgget");
        exit(1);
    }
    
    printf("Broker started (PID: %d)\n", getpid());
    printf("Waiting for messages...\n");
    
    signal(SIGINT, broker_signal_handler);
    signal(SIGTERM, broker_signal_handler);
    
    struct msgbuf msg;
    while (1) {
        if (msgrcv(msqid, &msg, sizeof(msg.mtext), 1, 0) == -1) {
            if (errno == EINTR) continue;
            perror("msgrcv");
            break;
        }
        
        char command[32];
        pid_t pid;
        char topic[MAX_TOPIC_LEN];
        char payload[MAX_PAYLOAD_LEN];
        
        if (!parse_message(msg.mtext, command, &pid, topic, payload)) {
            printf("Failed to parse: %s\n", msg.mtext);
            continue;
        }
        
        if (strcmp(command, "subscribe") == 0) {
            int already = 0;
            for (int i = 0; i < sub_count; i++) {
                if (subscriptions[i].pid == pid && 
                    strcmp(subscriptions[i].topic, topic) == 0) {
                    already = 1;
                    break;
                }
            }
            if (!already && sub_count < MAX_SUBSCRIPTIONS) {
                subscriptions[sub_count].pid = pid;
                strcpy(subscriptions[sub_count].topic, topic);
                sub_count++;
                printf("Subscriber %d subscribed to '%s'\n", pid, topic);
            }
        }
        else if (strcmp(command, "unsubscribe") == 0) {
            remove_subscription(pid, topic);
            printf("Subscriber %d unsubscribed from '%s'\n", pid, topic);
        }
        else if (strcmp(command, "register") == 0) {
            int already = 0;
            for (int i = 0; i < pub_count; i++) {
                if (publishers[i].pid == pid) {
                    already = 1;
                    break;
                }
            }
            if (!already && pub_count < MAX_PID_LIST) {
                publishers[pub_count].pid = pid;
                pub_count++;
                printf("Publisher %d registered on '%s'\n", pid, topic);
            }
        }
        else if (strcmp(command, "send") == 0) {
            printf("Message on '%s' from %d: %s\n", topic, pid, payload);
            
            pid_t subscribers[MAX_PID_LIST];
            int sub_found = 0;
            find_subscribers(topic, subscribers, &sub_found);
            
            struct msgbuf out_msg;
            snprintf(out_msg.mtext, sizeof(out_msg.mtext), "%s,%s", topic, payload);
            
            for (int i = 0; i < sub_found; i++) {
                out_msg.mtype = subscribers[i];
                if (msgsnd(msqid, &out_msg, strlen(out_msg.mtext) + 1, 0) == -1) {
                    perror("msgsnd");
                } else {
                    printf("Sent to subscriber %d\n", subscribers[i]);
                }
            }
            
            if (sub_found == 0) {
                printf("No subscribers for topic '%s'\n", topic);
            }
        }
    }
    
    msgctl(msqid, IPC_RMID, NULL);
}
