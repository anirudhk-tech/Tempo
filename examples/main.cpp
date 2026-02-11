#include "Tempo/tempo.h"

#include <cstdio>
#include <cstdint>

int main() {
  FILE* f = fopen("data/itch_sample", "rb");
  uint8_t type_buf[1];  

  while (fread(type_buf, 1, 1, f) == 1) {  
    char type = static_cast<char>(type_buf[0]);

    size_t msg_len;
    switch (type) {
        case 'A': case 'F': msg_len = 35; break; 
        case 'E': msg_len = 31; break;
        case 'X': msg_len = 23; break;
        case 'D': msg_len = 19; break;  
        case 'R': msg_len = 37; break;  
        case 'S': msg_len = 12; break;  
        default: msg_len = 35;
    }

    uint8_t msg_buf[40];
    msg_buf[0] = type;
    fread(msg_buf + 1, 1, msg_len - 1, f);

    if (type == 'A') {
        auto add = AddOrder::from_bytes(msg_buf);
        printf("A: %u shares ref=%lu\n", add.shares, add.order_ref);
    } else if (type == 'E') {
        auto exec = Executed::from_bytes(msg_buf);
        printf("E: exec %u on ref=%lu\n", exec.exec_shares, exec.order_ref);
    } else if (type == 'X') {
        auto cancel = Cancel::from_bytes(msg_buf);
        printf("X: cancel %u ref=%lu\n", cancel.cancel_shares, cancel.order_ref);
    }
  }   
}

