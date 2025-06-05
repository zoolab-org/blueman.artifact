#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "mutator/mutate_strategy.h"
#include "mutator/mutator.h"

#ifndef MIN
#  define MIN(_a,_b) ((_a) > (_b) ? (_b) : (_a))
#  define MAX(_a,_b) ((_a) > (_b) ? (_a) : (_b))
#endif /* !MIN */


static int urandom_dev_fd;
void selection_strategy_setup(PktselMode mode, uint64_t entry_index){}

// Choose a random block length for operations
static uint32_t choose_block_len(uint32_t limit) {
    if (limit <= 1) return 1;
    
    // Use a similar approach to AFL's choose_block_len
    uint32_t min_value, max_value;
    uint32_t rlim = rand() % 3;  // Emulating AFL's "run_over10m" logic in a simplified way
    
    switch (rlim) {
        case 0:  
            min_value = 1;
            max_value = 16;  // HAVOC_BLK_SMALL
            break;
            
        case 1:  
            min_value = 16;  // HAVOC_BLK_SMALL
            max_value = 32;  // HAVOC_BLK_MEDIUM
            break;
            
        default: 
            if (rand() % 10) {
                min_value = 32;   // HAVOC_BLK_MEDIUM
                max_value = 128;  // HAVOC_BLK_LARGE
            } else {
                min_value = 128;  // HAVOC_BLK_LARGE
                max_value = 1500; // HAVOC_BLK_XL
            }
    }
    
    if (min_value >= limit) min_value = 1;
    
    return min_value + rand() % (MIN(max_value, limit) - min_value + 1);
}
// Simplified AFL-style mutation with 8 specific operations
void afl_mutate_havoc(uint8_t* bytes, uint32_t* bytes_len_ptr, uint32_t max_bytes_len) {
    uint32_t bytes_len = *bytes_len_ptr;
    
    // Choose number of mutation rounds
    uint32_t round = rand() % ((bytes_len >> 1) + 1) + 1; // At least one mutation
    // uint32_t round = 1;

    for(int i = 0; i < round; i++) {
        // Eight mutation operations as requested
        switch(rand() % 8) {
            case 0: // 1. Flip a single bit
                {
                    if(bytes_len == 0) continue;
                    
                    // Choose a bit position (0 to bytes_len*8-1)
                    uint32_t bit_pos = rand() % (bytes_len << 3);
                    
                    // Identify the byte and bit within byte
                    uint8_t *byte = &bytes[bit_pos >> 3]; // Divide by 8 to get byte index
                    uint8_t bit_mask = 1 << (bit_pos & 7); // Get bit position within the byte
                    
                    // Flip the bit using XOR
                    *byte ^= bit_mask;
                }
                break;
                
            case 1: // 2. Set byte to interesting value
                {
                    if(bytes_len == 0) continue;
                    
                    uint32_t pos = rand() % bytes_len;
                    const int8_t interesting_8[] = { INTERESTING_8 };
                    bytes[pos] = interesting_8[rand() % ARRAY_SIZE(interesting_8)];
                }
                break;
                
            case 2: // 3. Set word to interesting value
                {
                    if(bytes_len < 2) continue;
                    
                    uint32_t pos = rand() % (bytes_len - 1); // Ensure space for 2 bytes
                    const int16_t interesting_16[] = { INTERESTING_8, INTERESTING_16 };
                    
                    // Set a 16-bit interesting value
                    uint16_t *word = (uint16_t*)&bytes[pos];
                    *word = interesting_16[rand() % ARRAY_SIZE(interesting_16)];
                }
                break;
            case 3: // 4. Set dword to interesting value
                {
                    if(bytes_len < 4) continue;
                    
                    uint32_t pos = rand() % (bytes_len - 3);
                    const int32_t interesting_32[] = { INTERESTING_8, INTERESTING_16, INTERESTING_32 };
                    
                    // Set a 32-bit interesting value with random endianness
                    if(rand() % 2) {
                        // Little endian
                        *(uint32_t*)&bytes[pos] = interesting_32[rand() % ARRAY_SIZE(interesting_32)];
                    } else {
                        // Big endian
                        uint32_t val = interesting_32[rand() % ARRAY_SIZE(interesting_32)];
                        bytes[pos] = (val >> 24) & 0xFF;
                        bytes[pos+1] = (val >> 16) & 0xFF;
                        bytes[pos+2] = (val >> 8) & 0xFF;
                        bytes[pos+3] = val & 0xFF;
                    }
                }
                break;    
            case 4: // 5. Randomly subtract from byte
                {
                    if(bytes_len == 0) continue;
                    
                    uint32_t pos = rand() % bytes_len;
                    uint16_t value = rand() % ARITH_MAX + 1; // 1 to ARITH_MAX
                    
                    // Subtract value from byte
                    bytes[pos] -= value;
                }
                break;
                
            case 5: // 5. Randomly subtract from word
                {
                    if(bytes_len < 2) continue;
                    
                    uint32_t pos = rand() % (bytes_len - 1); // Ensure space for 2 bytes
                    uint16_t value = rand() % ARITH_MAX + 1; // 1 to ARITH_MAX
                    
                    // Subtract value from word
                    uint16_t *word = (uint16_t*)&bytes[pos];
                    *word -= value;
                }
                break;
                
            case 6: // 6. Randomly add to byte
                {
                    if(bytes_len == 0) continue;
                    
                    uint32_t pos = rand() % bytes_len;
                    uint16_t value = rand() % ARITH_MAX + 1; // 1 to ARITH_MAX
                    
                    // Add value to byte
                    bytes[pos] += value;
                }
                break;
                
            case 7: // 7. Randomly add to word
                {
                    if(bytes_len < 2) continue;
                    
                    uint32_t pos = rand() % (bytes_len - 1); // Ensure space for 2 bytes
                    uint16_t value = rand() % ARITH_MAX + 1; // 1 to ARITH_MAX
                    
                    // Add value to word
                    uint16_t *word = (uint16_t*)&bytes[pos];
                    *word += value;
                }
                break;
             case 8: // 9. Randomly subtract from dword
                {
                    if(bytes_len < 4) continue;
                    
                    uint32_t pos = rand() % (bytes_len - 3); // Ensure space for 4 bytes
                    uint32_t value = rand() % ARITH_MAX + 1; // 1 to ARITH_MAX
                    
                    // Subtract value from dword with random endianness
                    if(rand() % 2) {
                        // Little endian
                        uint32_t *dword = (uint32_t*)&bytes[pos];
                        *dword -= value;
                    } else {
                        // Big endian - simplified implementation
                        uint32_t original = (bytes[pos] << 24) | (bytes[pos+1] << 16) | 
                                           (bytes[pos+2] << 8) | bytes[pos+3];
                        uint32_t result = original - value;
                        bytes[pos] = (result >> 24) & 0xFF;
                        bytes[pos+1] = (result >> 16) & 0xFF;
                        bytes[pos+2] = (result >> 8) & 0xFF;
                        bytes[pos+3] = result & 0xFF;
                    }
                }
                break;
            case 9: // 10. Randomly add to dword
                {
                    if(bytes_len < 4) continue;
                    
                    uint32_t pos = rand() % (bytes_len - 3); // Ensure space for 4 bytes
                    uint32_t value = rand() % ARITH_MAX + 1; // 1 to ARITH_MAX
                    
                    // Add value to dword with random endianness
                    if(rand() % 2) {
                        // Little endian
                        uint32_t *dword = (uint32_t*)&bytes[pos];
                        *dword += value;
                    } else {
                        // Big endian - simplified implementation
                        uint32_t original = (bytes[pos] << 24) | (bytes[pos+1] << 16) | 
                                           (bytes[pos+2] << 8) | bytes[pos+3];
                        uint32_t result = original + value;
                        bytes[pos] = (result >> 24) & 0xFF;
                        bytes[pos+1] = (result >> 16) & 0xFF;
                        bytes[pos+2] = (result >> 8) & 0xFF;
                        bytes[pos+3] = result & 0xFF;
                    }
                }
                break;
            case 10: // 11. Set random byte to random value (XOR with 1-255)
                {
                    if(bytes_len == 0) continue;
                    
                    uint32_t pos = rand() % bytes_len;
                    uint8_t xor_val = (rand() % 255) + 1; // 1-255 to avoid no-op
                    
                    bytes[pos] ^= xor_val;
                }
                break;

            case 11: // 12. Delete bytes
            case 12: // Make deletion more likely (13 is insertion)
                {
                    if(bytes_len < 2) continue;
                    
                    // Choose deletion size - don't delete too much
                    uint32_t del_len = choose_block_len(bytes_len / 2);
                    if(del_len >= bytes_len) del_len = 1;
                    
                    // Choose deletion position
                    uint32_t del_from = rand() % (bytes_len - del_len + 1);
                    
                    // Move remaining data
                    memmove(&bytes[del_from], &bytes[del_from + del_len], 
                           bytes_len - del_from - del_len);
                    
                    // Update length
                    *bytes_len_ptr -= del_len;
                    bytes_len = *bytes_len_ptr;
                }
                break;
            case 13: // 14. Clone bytes or insert constant
                {
                    // Check if we have room to insert data
                    uint32_t max_insertion = max_bytes_len - bytes_len;
                    if(max_insertion == 0) continue;
                    
                    // Decide how much to insert - limit to available space
                    uint32_t insert_len = choose_block_len(MIN(64, max_insertion));
                    if(insert_len == 0) insert_len = 1;
                    
                    // Choose insertion position
                    uint32_t insert_at = rand() % (bytes_len + 1);
                    
                    // Make room for the new bytes
                    memmove(&bytes[insert_at + insert_len], &bytes[insert_at], 
                           bytes_len - insert_at);
                    
                    // 75% chance to clone, 25% chance to insert constant
                    if(rand() % 4 < 3 && bytes_len) {
                        // Clone bytes from somewhere in the existing data
                        uint32_t clone_from = rand() % bytes_len;
                        
                        // Handle potential overlap
                        uint32_t clone_len = MIN(insert_len, bytes_len - clone_from);
                        
                        // Perform the clone
                        memcpy(&bytes[insert_at], &bytes[clone_from], clone_len);
                        
                        // If we couldn't clone enough, pad with a constant
                        if(clone_len < insert_len) {
                            memset(&bytes[insert_at + clone_len], rand() % 256, insert_len - clone_len);
                        }
                    } else {
                        // Insert constant bytes
                        uint8_t constant_val = rand() % 256;
                        memset(&bytes[insert_at], constant_val, insert_len);
                    }
                    
                    // Update length
                    *bytes_len_ptr += insert_len;
                    bytes_len = *bytes_len_ptr;
                }
                break;
                
            case 14: // 15. Overwrite bytes with random chunk or constant
                {
                    if(bytes_len < 2) continue;
                    
                    // Determine size to overwrite
                    uint32_t copy_len = choose_block_len(bytes_len - 1);
                    
                    // Choose source and destination
                    uint32_t copy_to = rand() % (bytes_len - copy_len + 1);
                    
                    // 75% chance to overwrite with existing data, 25% with constant
                    if(rand() % 4 < 3) {
                        uint32_t copy_from = rand() % (bytes_len - copy_len + 1);
                        
                        // Skip if source and destination are the same
                        if(copy_from != copy_to) {
                            // Handle potential overlap with a temporary buffer
                            uint8_t* temp_buf = malloc(copy_len);
                            memcpy(temp_buf, &bytes[copy_from], copy_len);
                            memcpy(&bytes[copy_to], temp_buf, copy_len);
                            free(temp_buf);
                        }
                    } else {
                        // Fill with a constant value
                        uint8_t constant_val = rand() % 256;
                        memset(&bytes[copy_to], constant_val, copy_len);
                    }
                }
                break;
                
        }
    }
}

// AFL-style mutation strategy without size limiting during mutation
int mutate_strategy(BLE_pkt* pkt) {
    uint32_t max_payload_len;
    uint32_t payload_len;

    // Set a random initial length if needed
    payload_len = pkt->max_payload_len > BLE_PKT_MAX_LEN ? BLE_PKT_MAX_LEN : pkt->max_payload_len; 
    pkt->len = rand() % (payload_len + 1) + 2 + 3;
    uint32_t len_tmp = pkt->len;
    
    // Apply havoc mutations without strict length enforcement
    afl_mutate_havoc(pkt->pkt, &len_tmp, BLE_PKT_MAX_LEN);
    pkt->len = len_tmp;  // Update the length after mutation

    // Mark as mutated
    pkt->mutated = 1;
    
    return 1;  // Assume mutation happened
}

int should_mutate_packet(int idx){
    // if(rand() % 100 < 50) return 0;
    return 1;
}

int mutate_pkt(BLE_pkt* pkt, int idx) {
    if (!should_mutate_packet(idx)) return 0;
    
    // Store original length
    uint32_t original_len = pkt->len;
    
    // Perform mutation
    int result = mutate_strategy(pkt);
    
    // Check if length exceeds maximum allowed, and truncate if necessary
    // This happens after mutation, as per requirement
    if (pkt->len > pkt->max_payload_len) {
        pkt->len = pkt->max_payload_len;
    }
    
    return result;
}

int pre_mutate(void) {
    uint32_t init_seed;
    read(urandom_dev_fd, &init_seed, sizeof(init_seed));
    srand(init_seed);
    return 0;
}

int mutator_init(void) {
    urandom_dev_fd = open("/dev/urandom", O_RDONLY);
    pre_mutate();
    return urandom_dev_fd;
}
