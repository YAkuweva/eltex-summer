#ifndef BROKER_SYSTEM_H
#define BROKER_SYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <time.h>

#define MAX_TOPIC_LEN 64
#define MAX_PAYLOAD_LEN 256
#define MAX_PID_LIST 100
#define MAX_SUBSCRIPTIONS 200

struct msgbuf {
    long mtype;
    char mtext[512];
};

struct subscription {
    pid_t pid;
    char topic[MAX_TOPIC_LEN];
};

struct publisher {
    pid_t pid;
};

extern int msqid;
extern struct subscription subscriptions[MAX_SUBSCRIPTIONS];
extern int sub_count;
extern struct publisher publishers[MAX_PID_LIST];
extern int pub_count;

void run_broker(void);
void broker_signal_handler(int sig);
void find_subscribers(char *topic, pid_t *found_pids, int *found_count);
void remove_subscription(pid_t pid, char *topic);
void remove_publisher(pid_t pid);

void run_publisher(char *topic);
void publisher_signal_handler(int sig);

void run_subscriber(char *topics_str);
void subscriber_signal_handler(int sig);

int parse_message(char *input, char *command, pid_t *pid, char *topic, char *payload);
void parse_received_message(char *input, char *topic, char *payload);

#endif
