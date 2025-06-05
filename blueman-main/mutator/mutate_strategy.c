#include "mutator/mutate_strategy.h"
#include <stdlib.h>
#include <stdio.h>
//const uint32_t flip_arr[] = {0b0, 0b1, 0b11, 0b111, 0b1111, 0b11111, 0b111111, 0b1111111, 0b11111111};
const uint8_t bits_arr[] = {0b0, 0b10000000, 0b11000000, 0b11100000, 0b11110000, 0b11111000, 0b11111100, 0b11111110, 0b11111111};
const int8_t  interesting_8[]  = { INTERESTING_8 };
const int16_t interesting_16[] = { INTERESTING_8, INTERESTING_16 };

inline void mutate_random_byte(uint8_t* bytes, uint32_t bytes_len){
    uint8_t byte, rnd;        
    rnd = rand() % bytes_len;
    byte = rand();
    bytes[rnd] = byte;
}

void mutate_rand_bits(uint8_t* bytes, uint32_t bytes_len, uint32_t S, uint8_t N){
    uint8_t *bits;
    uint8_t rnd_bits;

    if(N + S > (bytes_len << 3)) return;

    // shift S bits
    bits = &bytes[S >> 3];

    // replace N bits with random bits
    rnd_bits = rand() & bits_arr[N];
    *bits &= ~(bits_arr[N] >> (S & 0b111));
    *bits |= (rnd_bits >> (S & 0b111));

}

void mutate_random_bytes_insert(uint8_t* bytes, uint32_t bytes_len, uint32_t S, uint8_t N){
    if(N + S > (bytes_len << 3)) return;
	for(int i = 0 ; i < (N >> 3) ; i++){
		bytes[i + (S >> 3)] = rand();
    }
}

void mutate_arithmetic(uint8_t* bytes, uint32_t bytes_len, uint32_t S, uint8_t N){
    uint8_t *byte;
    uint16_t *word;
    uint16_t rnd;
    uint8_t is_add;

    if(N + S > (bytes_len << 3)) return;

    rnd = rand() % ARITH_MAX + 1;
    is_add = rand() & 1;
    switch(N >> 3){
        case 2:
            // word
            // shift S bits
            word = (uint16_t*)&bytes[S >> 3];

            // add or substract word
            *word = is_add ? *word + rnd : *word - rnd;
            break;
        case 1:
        default:
            // byte
            // shift S bits
            byte = &bytes[S >> 3];

            // add or substract 1 byte
            *byte = is_add ? *byte + rnd : *byte - rnd;
            break;
    }
}

void mutate_bits_flip(uint8_t* bytes, uint32_t bytes_len, uint32_t S, uint8_t N){
    uint8_t *bits;
    
    if(N + S > (bytes_len << 3)) return;

    // shift S bits
    bits = &bytes[S >> 3];

    // flip N bits
    *bits ^= (bits_arr[N] >> (S & 0b111));

}

void mutate_interest(uint8_t* bytes, uint32_t bytes_len, uint32_t S, uint8_t N){
    int8_t *byte;
    int16_t *word;

    if(N + S > (bytes_len << 3)) return;

    switch(N >> 3){
        case 2:
            // word
            // shift S bits
            word = (uint16_t*)&bytes[S >> 3];
            
            // add or substract word
            *word = interesting_16[rand() % ARRAY_SIZE(interesting_16)];
            break;
        case 1:
        default:
            // byte
            // shift S bits
            byte = &bytes[S >> 3];
            
            // add or substract 1 byte
            *byte = interesting_8[rand() % ARRAY_SIZE(interesting_8)];
            break;
    }
} 

void mutate_havoc(uint8_t* bytes, uint32_t bytes_len, uint32_t max_bytes_len){
	uint32_t round, S;
	uint8_t N;
    if(bytes_len > max_bytes_len) return;
	round = rand() % ((bytes_len >> 1) + 1); 
	for(int i = 0 ; i < round ; i++){
		switch(rand() % 6){
			case 0:
				mutate_random_byte(bytes, bytes_len);
				break;
			case 1:
				S = rand() % (bytes_len << 3);
				N = (rand() & 7) + 1;
				mutate_rand_bits(bytes, bytes_len, S, N);
				break;
			case 2:
				S = (rand() % bytes_len) << 3;
				N = rand() % (bytes_len - (S >> 3)) + 1;
                N = N << 3;
				mutate_random_bytes_insert(bytes, bytes_len, S, N);
				break;
			case 3:
				S = (rand() % bytes_len) << 3;
				N = (rand() % 2 + 1) << 3;
				mutate_arithmetic(bytes, bytes_len, S, N);
				break;
			case 4:
				S = rand() % (bytes_len << 3);
				N = (rand() & 7) + 1;
				mutate_bits_flip(bytes, bytes_len, S, N);
				break;
			case 5:
				S = (rand() % bytes_len) << 3;
				N = (rand() % 2 + 1) << 3;
                mutate_interest(bytes, bytes_len, S, N);
				break;
			default:		
				break;
		}	
	}	
}


