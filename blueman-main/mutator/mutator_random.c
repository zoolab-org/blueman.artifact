#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include "mutator/mutator.h"

static int urandom_dev_fd;
void selection_strategy_setup(PktselMode mode, uint64_t entry_index){}

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


int LL_advertising_mutator(BLE_pkt* pkt){
    uint32_t payload_len, max_payload_len, new_payload_len, max_mutation_round;
    uint8_t *LL_advertising_header;
    uint8_t *LL_advertising_payload;

    if(pkt->len <= 5) return 0;
    payload_len = pkt->len - 2 - 3;
    max_payload_len = pkt->max_payload_len > BLE_PKT_MAX_LEN ? BLE_PKT_MAX_LEN : pkt->max_payload_len;

    LL_advertising_header = GET_LL_ADV_HEADER(pkt->pkt);
    LL_advertising_payload = GET_LL_ADV_PAYLOAD(pkt->pkt);

    // mutate LL advertising header
    if(rand() % 100 >= 50){
        new_payload_len = rand() % max_payload_len + 1; 
    }else{
        new_payload_len = payload_len;
    }

    mutate_random_byte(LL_advertising_header, 1);
 
    if(new_payload_len == 0) goto end; 

    // mutate LL advertising payload 
    max_mutation_round = (rand() % new_payload_len + 3) / 3;
    for(int i = 0 ; i < max_mutation_round ; i++){
        mutate_random_byte(LL_advertising_payload, new_payload_len);
    }  

end:
    LL_advertising_header[1] = new_payload_len;
    pkt->len  = new_payload_len + 2 + 3;
    return new_payload_len;
}

int LL_data_mutator(BLE_pkt* pkt, int top_layer_type){
    uint32_t payload_len, max_payload_len, new_payload_len, max_mutation_round;
    uint8_t *LL_data_header;
    uint8_t *LL_data_payload;

    if(pkt->len <= 5) return 0;
    payload_len = pkt->len - 2 - 3;

    max_payload_len = pkt->max_payload_len > BLE_PKT_MAX_LEN ? BLE_PKT_MAX_LEN : pkt->max_payload_len;

    LL_data_header = GET_LL_DATA_HEADER(pkt->pkt);
    LL_data_payload = GET_LL_DATA_PAYLOAD(pkt->pkt);
    
    // start mutate LL data packet
    // mutate LL data header
    new_payload_len = rand() % max_payload_len + 1; 
    
    mutate_random_byte(LL_data_header, 1);
  
    if(new_payload_len == 0) goto end;

    // mutate LL data payload 
    max_mutation_round = (rand() % new_payload_len + 3) / 3;
    for(int i = 0 ; i < max_mutation_round ; i++){
        mutate_random_byte(LL_data_payload, new_payload_len);
    }  

end:
    LL_data_header[1] = new_payload_len;
    pkt->len  = new_payload_len + 2 + 3;
    return new_payload_len;
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
    int ret = 0, top_layer_type;
    if(rand() % 100 >= 98) return 0;
    if(is_data_packet(pkt)){
        top_layer_type = get_upper_layer_type_for_zephyr(pkt);
        ret = LL_data_mutator(pkt, top_layer_type);
    }else{
#ifndef DO_NOT_FUZZ_CONTROLLER
        ret = LL_advertising_mutator(pkt);    
#endif
    }

    if(ret != 0){
        pkt->mutated = 1;
    }

    return ret;
}

int mutate_pkt(BLE_pkt* pkt, int idx){
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
    return urandom_dev_fd;
}
