#include "raw_udp_capture.h"

int main(int argc, char *argv[]) {
    int raw_sock;
    int choice;
    char filename[256];
    
    (void)argc;
    (void)argv;
    
    printf("   UDP Packet Capture \n");
    
    init_filters();
    
    printf("Select filter:\n");
    printf("1. Chat messages (p 51000)\n");
    printf("2. DNS (p 53)\n");
    printf("3. NTP (p 123)\n");
    printf("Choice (1-3): ");
    
    if (scanf("%d", &choice) != 1) {
        choice = 1;
    }
    getchar();
    
    if (choice < 1 || choice > 3) {
        printf("Invalid choice. Using filter 1.\n");
        choice = 1;
    }
    
    current_filter_index = choice - 1;
    printf("\nUsing filter: %s\n", filters[current_filter_index].name);
    
    raw_sock = init_raw_socket();
    if (raw_sock < 0) {
        return 1;
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    capture_packets(raw_sock);
    
    close(raw_sock);
    
    if (packet_count > 0) {
        printf("\n Capture Results: \n");
        printf("1. Display all packets on screen\n");
        printf("2. Save to file\n");
        printf("3. Both\n");
        printf("Choice: ");
        if (scanf("%d", &choice) != 1) {
            choice = 2;
        }
        
        switch(choice) {
            case 1:
                print_all_packets();
                break;
            case 2:
                printf("Enter filename (1.txt): ");
                if (scanf("%255s", filename) != 1) {
                    strcpy(filename, "captured_packets.txt");
                }
                save_to_file(filename);
                break;
            case 3:
                print_all_packets();
                printf("Enter filename (1.txt): ");
                if (scanf("%255s", filename) != 1) {
                    strcpy(filename, "captured_packets.txt");
                }
                save_to_file(filename);
                break;
            default:
                printf("Invalid choice, saving to file.\n");
                save_to_file("captured_packets.txt");
        }
    } else {
        printf("No packets captured.\n");
    }
    
    cleanup();
    
    printf("\nProgram finished.\n");
    return 0;
}
