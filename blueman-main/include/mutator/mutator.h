#ifndef __MUTATOR_H_
#define __MUTATOR_H_
#include "mutate_strategy.h"
#include "pkt_record.h"

typedef enum {
    FIXED_PROB_10,
    FIXED_PROB_25,
    FIXED_PROB_50,
    FIXED_PROB_75,
    FIXED_PROB_100,
    SELECTIVE_25_75,
    SELECTIVE_75_25,
    RANDOM_PROB,
    MIXED_PROB,
} PktselMode;

typedef struct {
    PktselMode mode;
    float fixed_prob;              // For FIXED_PROB
    float first_prob; 
    float second_prob; 
    uint32_t entry_index; 
} PktselConfig;

int mutate_pkt(BLE_pkt* pkt, int idx);
void selection_strategy_setup(PktselMode mode, uint64_t entry_index);
int should_mutate_packet(int idx);

int pre_mutate(void);
int mutator_init(void);

#define FUZZ_LL_LAYER 0xd0
#define FUZZ_L2CAP_LAYER 0xd1
#define FUZZ_SM_LAYER 0xd2
#define FUZZ_ATT_LAYER 0xd3
#define FUZZ_UNKNOWN_LAYER 0xd4

#define ZEPHYR_GET_LL_ADV_PAYLOAD(ll_pkt) ((uint8_t*)ll_pkt + 2)
#define GET_LL_ADV_HEADER(ll_pkt) ((uint8_t*)ll_pkt)
#define GET_LL_ADV_PAYLOAD(ll_pkt) ZEPHYR_GET_LL_ADV_PAYLOAD(ll_pkt)

#ifndef CONFIG_BT_CTLR_DATA_LENGTH_CLEAR
    #define ZEPHYR_LL_HEADER_LEN 2
    #define ZEPHYR_GET_LL_DATA_PAYLOAD(ll_pkt, cp) ((uint8_t*)ll_pkt + ZEPHYR_LL_HEADER_LEN)
#else
    #define ZEPHYR_LL_HEADER_LEN 2
    #define ZEPHYR_GET_LL_DATA_PAYLOAD(ll_pkt, cp) ((uint8_t*)ll_pkt + ZEPHYR_LL_HEADER_LEN)
#endif
#define GET_LL_DATA_HEADER(ll_pkt) ((uint8_t*)ll_pkt)
#define GET_LL_DATA_PAYLOAD(ll_pkt) ZEPHYR_GET_LL_DATA_PAYLOAD((uint8_t*)ll_pkt, (*GET_LL_DATA_HEADER((uint8_t*)ll_pkt) >> 5) & 1)

#define L2CAP_HEADER_LEN 4
#define GET_L2CAP_HEADER(ll_pkt) GET_LL_DATA_PAYLOAD((uint8_t*)ll_pkt)
#define GET_L2CAP_PAYLOAD(ll_pkt) (GET_L2CAP_HEADER((uint8_t*)ll_pkt) + L2CAP_HEADER_LEN)

#define ATT_OPCODE_LEN 1
#define GET_ATT_OPCODE(ll_pkt) GET_L2CAP_PAYLOAD((uint8_t*)ll_pkt)
#define GET_ATT_PARAMS(ll_pkt) (GET_ATT_OPCODE((uint8_t*)ll_pkt) + ATT_OPCODE_LEN)

#define SM_CODE_LEN 1
#define GET_SM_CODE(ll_pkt) GET_L2CAP_PAYLOAD((uint8_t*)ll_pkt)
#define GET_SM_DATA(ll_pkt) (GET_SM_CODE((uint8_t*)ll_pkt) + SM_CODE_LEN)


#endif
