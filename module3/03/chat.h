#ifndef CHAT_H
#define CHAT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>

#define MAX_MSG_SIZE 256
#define MAX_MSG_COUNT 10
#define EXIT_PRIORITY 10
#define NORMAL_PRIORITY 1
#define EXIT_MSG "EXIT"
#define TIMEOUT_USEC 100000  

extern char queue_name_1[64];
extern char queue_name_2[64];
extern mqd_t queue_send;
extern mqd_t queue_recv;
extern bool is_creator;

int init_queues(const char *base_name);
void cleanup_queues(void);
void send_exit_message(void);
int receive_message(char *buffer, unsigned int *priority);
int send_message(const char *message, unsigned int priority);
int check_messages_available(void);
int check_keyboard_input(void);
int read_user_input(char *buffer, size_t size);

int setup_signal_handler(void);
void signal_handler(int sig);

void print_usage(const char *program_name);
void print_chat_started(void);

#endif 
