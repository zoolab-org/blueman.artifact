#ifndef __PKT_RECORD_H_
#define __PKT_RECORD_H_

#include <stdio.h>
#include "interceptor.h"

typedef struct{
    size_t count;
    size_t max_count;
    BLE_pkt pkts_buf[0];
}pkt_record;


pkt_record * create_pkt_seq_buf(size_t);
int add_pkt_to_seq_buf(pkt_record * , const BLE_pkt * );
int save_seq_buf_to_file1(pkt_record *, char * );
int save_seq_buf_to_file2(pkt_record * , char* );
void* mmap_seq_buf_file(char*, size_t);
int munmap_seq_buf_file(void *, size_t);
int copy_file_to_seq_buf(pkt_record *, char*);
uint32_t get_pkt_seq_total_len(pkt_record *);
int destroy_pkt_seq_buf(pkt_record * , size_t);
int init_pkt_seq_buf(pkt_record*);
#endif
