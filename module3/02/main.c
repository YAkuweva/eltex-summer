#include "broker.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage:\n");
        printf("  %s -b                    (broker mode)\n", argv[0]);
        printf("  %s -p <topic>            (publisher mode)\n", argv[0]);
        printf("  %s -s <topic1,topic2,...> (subscriber mode)\n", argv[0]);
        printf("\nExamples:\n");
        printf("  %s -b\n", argv[0]);
        printf("  %s -p news\n", argv[0]);
        printf("  %s -s news,sports,weather\n", argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "-b") == 0) {
        run_broker();
    }
    else if (strcmp(argv[1], "-p") == 0) {
        if (argc < 3) {
            printf("Publisher: Please specify topic\n");
            printf("Usage: %s -p <topic>\n", argv[0]);
            return 1;
        }
        run_publisher(argv[2]);
    }
    else if (strcmp(argv[1], "-s") == 0) {
        if (argc < 3) {
            printf("Subscriber: Please specify topics (comma-separated)\n");
            printf("Usage: %s -s <topic1,topic2,...>\n", argv[0]);
            return 1;
        }
        run_subscriber(argv[2]);
    }
    else {
        printf("Unknown option: %s\n", argv[1]);
        printf("Use -b (broker), -p (publisher), or -s (subscriber)\n");
        return 1;
    }
    
    return 0;
}
