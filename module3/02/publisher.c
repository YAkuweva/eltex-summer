#include "broker.h"

int publisher_msqid;
pid_t publisher_pid;

void publisher_signal_handler(int sig) {
    printf("\nPublisher: Exiting...\n");
    exit(0);
}

void run_publisher(char *topic) {
    publisher_pid = getpid();
    
    key_t key = ftok("/tmp", 'M');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }
    
    publisher_msqid = msgget(key, 0666);
    if (publisher_msqid == -1) {
        perror("msgget");
        printf("No broker running. Start broker first (./broker_system -b)\n");
        exit(1);
    }
    
    signal(SIGINT, publisher_signal_handler);
    signal(SIGTERM, publisher_signal_handler);
    
    printf("Publisher (PID: %d) on topic '%s'\n", publisher_pid, topic);
    
    struct msgbuf reg_msg;
    reg_msg.mtype = 1;
    snprintf(reg_msg.mtext, sizeof(reg_msg.mtext), "register,%d,%s", publisher_pid, topic);
    if (msgsnd(publisher_msqid, &reg_msg, strlen(reg_msg.mtext) + 1, 0) == -1) {
        perror("msgsnd");
        exit(1);
    }
    
    printf("Enter messages (or 'quit' to exit):\n");
    char input[256];
    
    while (1) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        input[strcspn(input, "\n")] = 0;
        
        if (strcmp(input, "quit") == 0) break;
        if (strlen(input) == 0) continue;
        
        struct msgbuf send_msg;
        send_msg.mtype = 1;
        snprintf(send_msg.mtext, sizeof(send_msg.mtext), 
                "send,%d,%s,%s", publisher_pid, topic, input);
        
        if (msgsnd(publisher_msqid, &send_msg, strlen(send_msg.mtext) + 1, 0) == -1) {
            if (errno == EIDRM) {
                printf("Broker exited. Shutting down.\n");
                break;
            }
            perror("msgsnd");
            break;
        }
        printf("Sent: %s\n", input);
    }
}
