#pragma once

#include <cstdint>
#include <cstring>
#include <arpa/inet.h>

inline double price_to_double(uint32_t raw) {
  return ntohl(raw) / 10000.0;
}

struct __attribute__((packed)) AddOrder {
  char type;
  uint16_t stock_loc;
  uint16_t tracking;
  uint32_t timestamp_ns;
  uint64_t order_ref;
  char side;
  uint32_t shares;
  char stock[8];
  uint32_t price;

  static AddOrder from_bytes(const uint8_t* buf) {
    AddOrder msg;
    std::memcpy(&msg, buf, sizeof(AddOrder));
    msg.stock_loc = ntohs(msg.stock_loc);
    msg.tracking = ntohs(msg.tracking);
    msg.timestamp_ns = ntohl(msg.timestamp_ns);
    msg.order_ref = ntohl(msg.order_ref & 0xFFFFFFFFULL);
    msg.shares = ntohl(msg.shares);
    msg.price = ntohl(msg.price);
    return msg;
  }
};

struct __attribute__((packed)) Executed {
  char type;
  uint16_t stock_loc;
  uint16_t tracking;
  uint32_t timestamp_ns;
  uint64_t order_ref;
  uint32_t exec_shares;
  uint64_t match_num;

  static Executed from_bytes(const uint8_t* buf) {
    Executed msg; memcpy(&msg, buf, sizeof(Executed));
    
    uint16_t temp; memcpy(&temp, &msg.stock_loc, 2); msg.stock_loc = ntohs(temp);
    memcpy(&temp, &msg.tracking, 2); msg.tracking = ntohs(temp);
    
    uint32_t temp32; memcpy(&temp32, &msg.timestamp_ns, 4); msg.timestamp_ns = ntohl(temp32);
    memcpy(&temp32, &msg.exec_shares, 4); msg.exec_shares = ntohl(temp32);
    
    msg.order_ref = ntohll(msg.order_ref); 
    msg.match_num = ntohll(msg.match_num);
    
    return msg;
  }
};

struct __attribute__ ((packed)) Cancel {
  char type;
  uint16_t stock_loc;
  uint16_t tracking;
  uint32_t timestamp_ns;
  uint64_t order_ref;
  uint32_t cancel_shares;

  static Cancel from_bytes(const uint8_t* buf) {
    Cancel msg; memcpy(&msg, buf, sizeof(Cancel));
    
    uint16_t temp; memcpy(&temp, &msg.stock_loc, 2); msg.stock_loc = ntohs(temp);
    memcpy(&temp, &msg.tracking, 2); msg.tracking = ntohs(temp);
    
    uint32_t temp32; memcpy(&temp32, &msg.timestamp_ns, 4); msg.timestamp_ns = ntohl(temp32);
    memcpy(&temp32, &msg.cancel_shares, 4); msg.cancel_shares = ntohl(temp32);
    
    msg.order_ref = ntohll(msg.order_ref); 
    
    return msg;
  }
};

inline AddOrder parse_add(const uint8_t* buf) { return AddOrder::from_bytes(buf); }

struct TempoHandler {
  void (*on_add) (const AddOrder&);
  void (*on_exec) (const Executed&);
  void (*on_cancel) (const Cancel&);
};

void parse_stream(const uint8_t* data, size_t len, const TempoHandler& h) {
  size_t i = 0;
}


