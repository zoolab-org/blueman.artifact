#ifndef __COVERAGE_H_
#define __COVERAGE_H_
#include "utils.h"
#include "config.h"
#include "pkt_record.h"
struct queue_entry {
    char* fname; 
    uint32_t count;
    uint32_t len;                            

    uint8_t was_fuzzed;
    uint8_t has_new_cov;
    uint8_t favored;

    uint32_t bitmap_size;

    uint64_t exec_us;

    struct list_head list;
};

char* get_shm_str(void);
int get_shm_id(void);
void setup_shm(void);
void clean_trace_bits(void);
uint32_t corpus_get_coverage(void);
uint8_t save_if_interesting(char*, pkt_record *, uint32_t);
void load_corpus_to_queue(char*);
uint32_t get_queue_length(void);
void print_queue_entry_info(struct queue_entry*);
struct queue_entry* get_queue_head(void);
struct queue_entry* get_queue_tail(void);
struct queue_entry* get_queue_next(struct queue_entry*);
uint32_t get_queue_size();
uint32_t get_total_coverage();

#define FF(_b)  (((uint64_t)0xff) << ((_b) << 3))
#define MAP_SIZE 2 << 16
#define SHM_ENV_VAR "__AFL_SHM_ID"

#endif
