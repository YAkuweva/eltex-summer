#include "broker.h"

int parse_message(char *input, char *command, pid_t *pid, char *topic, char *payload) {
    char temp[512];
    strcpy(temp, input);
    
    char *p1 = strchr(temp, ',');
    if (p1 == NULL) return 0;
    *p1 = '\0';
    strcpy(command, temp);
    
    char *p2 = p1 + 1;
    char *p3 = strchr(p2, ',');
    if (p3 == NULL) return 0;
    *p3 = '\0';
    *pid = atoi(p2);
    
    char *p4 = p3 + 1;
    char *p5 = strchr(p4, ',');
    if (p5 == NULL) {
        strcpy(topic, p4);
        payload[0] = '\0';
        return 1;
    }
    *p5 = '\0';
    strcpy(topic, p4);
    strcpy(payload, p5 + 1);
    
    return 1;
}

void parse_received_message(char *input, char *topic, char *payload) {
    char temp[512];
    strcpy(temp, input);
    
    char *p = strchr(temp, ',');
    if (p == NULL) {
        strcpy(topic, "unknown");
        strcpy(payload, input);
        return;
    }
    
    *p = '\0';
    strcpy(topic, temp);
    strcpy(payload, p + 1);
}
