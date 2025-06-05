#include "btfuzz.h"
#include <stdint.h>
#include <stddef.h>
#include <sys/ipc.h>

static int qid;
int create_msg_queue(){
    qid = msgget(MSG_QUEUE_KEY, IPC_CREAT | 0777);

    return qid;
}

int delete_msg_queue(){
    return msgctl(qid, IPC_RMID, 0);
}

int recv_ble_pkt(BLE_pkt* pkt){
    msg_fuzz_pkt msg;
    msg.type = MSG_FUZZ_RECV_PKT;
    msg.pkt = *pkt;

    return msgsnd(qid, &msg, sizeof(msg.pkt), 0);
}

int recv_ble_pkt_ack(BLE_pkt* pkt){
    msg_fuzz_pkt msg;
    msg.type = MSG_FUZZ_RECV_PKT_ACK;
    msg.pkt = *pkt;


    return msgsnd(qid, &msg, sizeof(msg.pkt), 0);
}

