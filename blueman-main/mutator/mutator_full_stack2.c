#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include "mutator/mutate_strategy.h"
#include "mutator/mutator.h"

static int urandom_dev_fd;

static PktselConfig pkt_selection_config;

// custom packet parser for zephyr
int get_upper_layer_type_for_zephyr(BLE_pkt* pkt){
    int32_t LL_payload_len;
    uint8_t* ll_header, *ll_payload,* l2cap_header;
    uint16_t cid;
    uint8_t llid;

    ll_header = GET_LL_DATA_HEADER(pkt->pkt);
    llid = ll_header[0] & 0b11;
    LL_payload_len = ll_header[1]; 
    ll_payload = GET_LL_DATA_PAYLOAD(pkt->pkt);  

    // if LLID == 0b11 then this packet is LL Control PDU
    if(llid == 0b11){
        return FUZZ_LL_LAYER; 
    }
    // Since header of L2CAP layer is 4 bytes, LL_payload_len should not be less than 4
    if(LL_payload_len - 4 < 0){
        return FUZZ_LL_LAYER;
    }

    if(llid == 0b10){
        //start of l2cap fragment
        l2cap_header = ll_payload;
        cid = ((uint16_t*)l2cap_header)[1];

        if(cid == 0x4){
            // att 
            return FUZZ_ATT_LAYER;
        }else if(cid == 0x6){
            // smp 
            return FUZZ_SM_LAYER;
        }else{
            // unknown upper layer (others, like Dynamic allocated) or L2CAP Signaling channel (0x5) 
            // so we treat it as L2CAP layer
            return FUZZ_L2CAP_LAYER;
        }
    }else{
        return FUZZ_L2CAP_LAYER;
    }
}
#define ATT_OP_MAPS_LEN 35
static uint8_t att_op_maps[ATT_OP_MAPS_LEN] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
                                               0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 
                                               0x0f, 0x10, 0x11, 0x12, 0x13, 0x05, 0x16,
                                               0x17, 0x18, 0x19, 0x20, 0x21, 0x22, 0x23,
                                               0x24, 0x1b, 0x1d, 0x1e, 0xd2, 0xd3, 0x1a};
int ATT_mutator(BLE_pkt* pkt, int top_layer_type){
    uint8_t *att_opcode, *att_params;
    int32_t att_params_len;
    int32_t LL_payload_len;

    LL_payload_len = pkt->len - 2 - 3;
    att_params_len = LL_payload_len - L2CAP_HEADER_LEN - ATT_OPCODE_LEN; 
    if(att_params_len <= 0) goto end;

    att_opcode = GET_ATT_OPCODE(pkt); 
    att_params = GET_ATT_PARAMS(pkt); 
    // mutate att opcode
    if(rand() % 100 >= 30){
        *att_opcode = att_op_maps[rand() % ATT_OP_MAPS_LEN];
    }

    // mutate att parameters
    mutate_havoc(att_params, att_params_len, BLE_PKT_MAX_LEN);
end:
    return 1;  
}

int SM_mutator(BLE_pkt* pkt, int top_layer_type){
    uint8_t *sm_code, *sm_data;
    int32_t sm_data_len;
    int32_t LL_payload_len;

    LL_payload_len = pkt->len - 2 - 3;
    sm_data_len = LL_payload_len - L2CAP_HEADER_LEN - SM_CODE_LEN; 
    if(sm_data_len <= 0) goto end;

    sm_code = GET_SM_CODE(pkt); 
    sm_data = GET_SM_DATA(pkt); 
    // mutate sm code
    if(rand() % 100 < 70){
        *sm_code = rand() % 0xf;
    }

    // mutate att parameters
    mutate_havoc(sm_data, sm_data_len, BLE_PKT_MAX_LEN);
end:
    return 1;  
}

#define CID_MAPS_LEN 7
static uint16_t cid_maps[CID_MAPS_LEN] = {0x4, 0x5, 0x6, 0x25, 0x27, 0x40, 0x41};
int L2CAP_mutator(BLE_pkt* pkt, int top_layer_type){
    int32_t payload_len, new_payload_len;
    uint16_t cid;
    uint8_t *l2cap_header;
    uint8_t *l2cap_payload;
    int32_t LL_payload_len;

    LL_payload_len = pkt->len - 2 - 3;
    l2cap_header = GET_L2CAP_HEADER(pkt->pkt);
    payload_len = LL_payload_len - L2CAP_HEADER_LEN;
    cid = ((uint16_t*)l2cap_header)[1];

    l2cap_payload = GET_L2CAP_PAYLOAD(pkt->pkt);

    //  If max_payload_len is less than 0, we should not mutate any field.
    if(payload_len <= 0) return 0;

    // mutate length of L2CAP payload
    if(rand() % 100 < 20){
        new_payload_len = rand() % (payload_len + 1); 
    }else{
        new_payload_len = payload_len;
    }

    if(new_payload_len == 0) goto end;

    // start mutate L2CAP packet
    // mutate L2CAP header
    if(rand() % 100 < 20){
        // mutate Channel ID
        ((uint16_t*)l2cap_header)[1] = cid_maps[rand() % CID_MAPS_LEN];
    }

    // mutate l2cap payload 
    mutate_havoc(l2cap_payload, new_payload_len, BLE_PKT_MAX_LEN);
end:
    return 1;  
}

int LL_advertising_mutator(BLE_pkt* pkt){
    int32_t payload_len;
    uint8_t *LL_advertising_header;
    uint8_t *LL_advertising_payload;

    payload_len = pkt->len - 2 - 3;
    if(payload_len <= 0) return 0;
    LL_advertising_header = GET_LL_ADV_HEADER(pkt->pkt);
    LL_advertising_payload = GET_LL_ADV_PAYLOAD(pkt->pkt);

    // mutate LL advertising header
    if(rand() % 100 < 10){
        // mutate PDU Type
        mutate_rand_bits(LL_advertising_header, 2, 4, 4);
    }

    // mutate LL advertising payload 
    mutate_havoc(LL_advertising_payload, payload_len, BLE_PKT_MAX_LEN);
    return 1;
}

int LL_data_mutator(BLE_pkt* pkt, int top_layer_type){
    int32_t payload_len;
    uint8_t *LL_data_header;
    uint8_t *LL_data_payload;

    payload_len = pkt->len - 2 - 3;
    if(payload_len <= 0) return 0;
    LL_data_header = GET_LL_DATA_HEADER(pkt->pkt);
    LL_data_payload = GET_LL_DATA_PAYLOAD(pkt->pkt);
    
    // start mutate LL data packet
    // mutate LL data header
    if(rand() % 100 < 30){
        // mutate LLID
        mutate_rand_bits(LL_data_header, 2, 6, 2);
        // mutate MD and RFU
        mutate_rand_bits(LL_data_header, 2, 0, 4);
    }
  
    // mutate LL data payload 
    mutate_havoc(LL_data_payload, payload_len, BLE_PKT_MAX_LEN);

    return 1;
}

int is_data_packet(BLE_pkt* pkt){
    uint32_t access_address;

    access_address = pkt->address;
    if(access_address == 0x8E89BED6){
        return 0;
    }else{
        return 1;
    }
}

int mutate_strategy(BLE_pkt* pkt){
    int ret = 0, top_layer_type, r;
    uint32_t max_payload_len;
    uint8_t *LL_data_header;
    uint8_t *LL_advertising_header;
    uint8_t old_payload_len;

    // First, mutate length of LL packet
    if(rand() % 100 < 50){
        max_payload_len = pkt->max_payload_len > BLE_PKT_MAX_LEN ? BLE_PKT_MAX_LEN : pkt->max_payload_len; 
        pkt->len = rand() % (max_payload_len + 1) + 2 + 3;
    }
    old_payload_len = pkt->len;

    if(is_data_packet(pkt)){
        top_layer_type = get_upper_layer_type_for_zephyr(pkt);
        r = rand() % 100;
        switch(top_layer_type){
            case FUZZ_LL_LAYER:
                if(r < 25){
                    ret = LL_data_mutator(pkt, top_layer_type);
                }
                break;
            case FUZZ_L2CAP_LAYER:
                if(r < 25){
                    ret = LL_data_mutator(pkt, top_layer_type);
                }else if(r < 50){
                    ret = L2CAP_mutator(pkt, top_layer_type);
                }
                break;
            case FUZZ_ATT_LAYER:
                if(r < 25){
                    ret = LL_data_mutator(pkt, top_layer_type);
                }else if(r < 50){
                    ret = L2CAP_mutator(pkt, top_layer_type);
                }else if(r < 75){
                    ret = ATT_mutator(pkt, top_layer_type);
                }
                break;
            case FUZZ_SM_LAYER:
                if(r < 25){
                    ret = LL_data_mutator(pkt, top_layer_type);
                }else if(r < 50){
                    ret = L2CAP_mutator(pkt, top_layer_type);
                }else if(r < 75){
                    ret = SM_mutator(pkt, top_layer_type);
                }
            default:
                break;
        }
        LL_data_header = GET_LL_DATA_HEADER(pkt->pkt);
        LL_data_header[1] = pkt->len - 2 - 3;
#ifndef DO_NOT_FUZZ_CONTROLLER
    }else{
        ret = LL_advertising_mutator(pkt);
        LL_advertising_header = GET_LL_ADV_HEADER(pkt->pkt);
        LL_advertising_header[1] = pkt->len - 2 - 3;
#endif
    }
    // Payload of packet is modifed or length of packet is changed
    if(ret != 0 || old_payload_len != pkt->len){
        pkt->mutated = 1;
    }

    return ret;
}


void selection_strategy_setup(PktselMode mode, uint64_t entry_index){
    if (mode < FIXED_PROB_10 || mode > MIXED_PROB) {
        printf("Invalid mode %d, fallback to FIXED_PROB_50\n", mode);
        mode = FIXED_PROB_50;
    }
    pkt_selection_config.mode = mode;
    pkt_selection_config.entry_index = entry_index;
    switch (mode) {
        case FIXED_PROB_10:
            pkt_selection_config.fixed_prob = 0.10f;
            break;
        case FIXED_PROB_25:
            pkt_selection_config.fixed_prob = 0.25f;
            break;
        case FIXED_PROB_50:
            pkt_selection_config.fixed_prob = 0.50f;
            break;
        case FIXED_PROB_75:
            pkt_selection_config.fixed_prob = 0.75f;
            break;
        case FIXED_PROB_100:
            pkt_selection_config.fixed_prob = 1.00f;
            break;
        case SELECTIVE_25_75:
            pkt_selection_config.first_prob = 0.25f;
            pkt_selection_config.second_prob = 0.75f;
            break;
        case SELECTIVE_75_25:
            pkt_selection_config.first_prob = 0.75f;
            pkt_selection_config.second_prob = 0.25f;
            break;
        case RANDOM_PROB:
        case MIXED_PROB:
            // no config needed
            break;
    }
}

int should_mutate_packet(int idx){
    float prob;
    switch(pkt_selection_config.mode) {
        case FIXED_PROB_10:
        case FIXED_PROB_25:
        case FIXED_PROB_50:
        case FIXED_PROB_75:
        case FIXED_PROB_100:
            prob = pkt_selection_config.fixed_prob;
            break;
        case RANDOM_PROB:
            int r = rand() % 91;     // 0 ~ 90
            prob = (r + 10) / 100.0f;  // 0.10 ~ 1.00
            break;
        case SELECTIVE_25_75:
        case SELECTIVE_75_25:
            prob = (idx >= pkt_selection_config.entry_index) ?
                   pkt_selection_config.first_prob : pkt_selection_config.second_prob;
            break;
        default:
            prob = 0.5;
            break;
    }
    
    float r =  ((float)rand() / RAND_MAX);
    return r < prob;
}


int mutate_pkt(BLE_pkt* pkt, int idx){
    if(pkt->len == 2 + 3) return 0;

    if(!should_mutate_packet(idx)){
        return 0;
    }
   return mutate_strategy(pkt); 
}

int pre_mutate(void){
    uint32_t init_seed;
    read(urandom_dev_fd, &init_seed, sizeof(init_seed));
    srand(init_seed);
    return 0;
}

int mutator_init(void){
    // path
    urandom_dev_fd = open("/dev/urandom", O_RDONLY);
    pre_mutate();

#ifndef PKTSEL
    // Default mode is FIXED_PROB_50
    pkt_selection_config.mode = FIXED_PROB_50;
    pkt_selection_config.fixed_prob = 0.50f;
#endif
    return urandom_dev_fd;
}



