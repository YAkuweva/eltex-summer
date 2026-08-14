#include "utils.h"

int is_valid_nickname(const char *nickname) {
    if (nickname == NULL || strlen(nickname) == 0) {
        return 0;
    }
    for (int i = 0; nickname[i] != '\0'; i++) {
        if (!isalnum(nickname[i]) && nickname[i] != '_') {
            return 0;
        }
    }
    return 1;
}

void safe_strcpy(char *dest, const char *src, int size) {
    if (dest == NULL || src == NULL || size <= 0) {
        return;
    }
    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
}

int read_user_input(char *buffer, int buffer_size) {
    if (fgets(buffer, buffer_size, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
        return 1;
    }
    return 0;
}

int is_my_message(const char *message) {
    return strstr(message, nickname) != NULL;
}

void display_message(const char *message) {
    printf("%s\n", message);
    fflush(stdout);
}
