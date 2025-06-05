#ifndef __INTERCEPTOR_H__
#define __INTERCEPTOR_H__
#include <stdint.h>
#include <stddef.h>
#include <sys/msg.h> 

#define BLE_PKT_MAX_LEN 255

#define MSG_QUEUE_KEY 0x878787

enum MSG_FUZZ{
	MSG_FUZZ_RECV_PKT = 1,
	MSG_FUZZ_RECV_PKT_ACK, 
};


typedef struct {
    uint32_t sid; // sequence id
    uint32_t device_id;
	uint32_t mutated;
    uint32_t address;
    uint32_t len;   // size of packet (2 + payload's length + 3)
	uint32_t max_payload_len;
    uint8_t pkt[BLE_PKT_MAX_LEN];    
}__attribute__((packed)) BLE_pkt;

typedef struct {
    long type;
    BLE_pkt pkt;
}msg_fuzz_pkt;

typedef struct {
    long type;
    long flags;
}msg_fuzz_event;
int create_msg_queue();
int delete_msg_queue();
int start_recv_pkts(long);
int recv_ble_pkt(BLE_pkt*);
int recv_ble_pkt_ack(BLE_pkt*);
int end_recv_pkts(long);

#endif
