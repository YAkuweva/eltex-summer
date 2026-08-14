#ifndef NETWORK_H
#define NETWORK_H

#include "chat.h"

int init_socket(void);

int send_broadcast(const char *message);

int receive_message(char *buffer, int buffer_size, struct sockaddr_in *sender_addr);

void send_join_message(void);
void send_leave_message(void);

void send_chat_message(const char *text);

void print_network_info(void);

void close_socket(void);

#endif 
