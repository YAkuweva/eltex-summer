#include "common.h"

uint16_t udp_checksum(struct iphdr *ip_header, struct udphdr *udp_header, 
                       const char *data, int data_len) {
    uint32_t sum = 0;
    uint16_t *ptr;
    unsigned int i;
    int total_len = sizeof(struct udphdr) + data_len;
    
    struct pseudo_header {
        uint32_t src_addr;
        uint32_t dst_addr;
        uint8_t zero;
        uint8_t protocol;
        uint16_t udp_length;
    } pseudo;
    
    pseudo.src_addr = ip_header->saddr;
    pseudo.dst_addr = ip_header->daddr;
    pseudo.zero = 0;
    pseudo.protocol = IPPROTO_UDP;
    pseudo.udp_length = htons(total_len);
    
    ptr = (uint16_t*)&pseudo;
    for (i = 0; i < sizeof(pseudo)/sizeof(uint16_t); i++) {
        sum += ntohs(ptr[i]);
        if (sum & 0x10000) {
            sum = (sum + 1) & 0xFFFF;
        }
    }
    
    ptr = (uint16_t*)udp_header;
    for (i = 0; i < sizeof(struct udphdr)/sizeof(uint16_t); i++) {
        sum += ntohs(ptr[i]);
        if (sum & 0x10000) {
            sum = (sum + 1) & 0xFFFF;
        }
    }
    
    ptr = (uint16_t*)data;
    for (i = 0; i < (unsigned int)data_len/2; i++) {
        sum += ntohs(ptr[i]);
        if (sum & 0x10000) {
            sum = (sum + 1) & 0xFFFF;
        }
    }
    
    if (data_len % 2) {
        sum += ntohs(((uint16_t*)data)[data_len/2] & 0xFF00);
        if (sum & 0x10000) {
            sum = (sum + 1) & 0xFFFF;
        }
    }
    
    return htons(~sum);
}
