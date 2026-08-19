#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <time.h>
#include <fcntl.h>

#define BUFFER_SIZE 256
#define MAX_DRIVERS 100
#define MSG_MAX 64
#define REQUEST_TIMEOUT_MS 1000

typedef struct {
    pid_t pid;
    int fd;
    int active;
    char status[BUFFER_SIZE];
    int remaining_time;
    time_t busy_until;
} DriverInfo;

#endif
