#ifndef UTILS_H
#define UTILS_H

#include "chat.h"

int is_valid_nickname(const char *nickname);

void safe_strcpy(char *dest, const char *src, int size);

int read_user_input(char *buffer, int buffer_size);

int is_my_message(const char *message);

void display_message(const char *message);

#endif 
