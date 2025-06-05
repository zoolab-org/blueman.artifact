#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "pkt_record.h"
#include "debug.h"


pkt_record * create_pkt_seq_buf(size_t n){
    size_t buf_size = sizeof(pkt_record) + n * sizeof(BLE_pkt);
    pkt_record * pkt_rec = mmap(NULL, buf_size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pkt_rec->count = 0;
    pkt_rec->max_count = n;
    return pkt_rec;
}

int init_pkt_seq_buf(pkt_record* pkt_rec){
    pkt_rec->count = 0;
    return 0;
}

int add_pkt_to_seq_buf(pkt_record * pkt_rec, const BLE_pkt* pkt){
    if(pkt_rec->max_count > pkt_rec->count + 1){
        pkt_rec->pkts_buf[pkt_rec->count++] = *pkt;
        return 0;
    }else{
        return -1;
    }
}

uint32_t get_pkt_seq_total_len(pkt_record * pkt_rec){
	uint32_t ret = 0;
	for(int i = 0 ; i< pkt_rec->count ; i++){
		ret += pkt_rec->pkts_buf[i].len;
	}
	return ret;
}

int copy_file_to_seq_buf1(pkt_record * pkt_rec, char* filename){
    uint64_t tmp;
    BLE_pkt* pkt;
    char *mem;
    size_t mem_len, n = 0;
    int fd = open(filename, O_RDONLY); 
    int count = 0;
    if(fd < 0){
         DEBUG("%s open failed\n", __FUNCTION__);
         exit(-1);
    }
    mem_len = (size_t) lseek(fd, 0, SEEK_END);
    mem = mmap(NULL, mem_len, PROT_READ, MAP_SHARED, fd, 0);

    if(mem == NULL){
         DEBUG("%s mmap failed\n", __FUNCTION__);
         exit(-1);
    }
    while(mem_len > n){
        pkt = &pkt_rec->pkts_buf[count];
        memcpy(&pkt->address, &mem[n], 4);
        n += 4;

        memcpy(&tmp, &mem[n], 8);
        pkt->max_payload_len = (uint32_t)tmp;
        n += 8;

        memcpy(&tmp, &mem[n], 8);
        pkt->len = (uint32_t)(tmp + 2 + 3);
        n += 8;

        memcpy(pkt->pkt, &mem[n], pkt->len);
        n += pkt->len;
        if(n < 0 || n > mem_len){
            DEBUG("%s malform seq buf\n", __FUNCTION__);
            break;
        }
       
        memcpy(&pkt->device_id, &mem[n], 4);
        n += 4;
        count++;
    } 
    pkt_rec->count = count;
    
    
    munmap(mem, mem_len);
    close(fd);

    return count;
}

int save_seq_buf_to_file1(pkt_record * pkt_rec, char* filename){
    uint64_t tmp;
    BLE_pkt* pkt;
    FILE *file = fopen(filename, "w+"); 
    int i;
    if(file == NULL){
         DEBUG("%s fopen failed\n", __FUNCTION__);;
         exit(-1);
    }
    for(i = 0 ; i < pkt_rec->count ; i++){
        pkt = &pkt_rec->pkts_buf[i];
        fwrite(&pkt->address, 1, 4, file);

        tmp = pkt->max_payload_len;
        fwrite(&tmp, 1, 8, file);

        // payload's length
        tmp = pkt->len - 2 - 3;
        fwrite(&tmp, 1, 8, file);


        fwrite(pkt->pkt, 1, pkt->len, file);
       
        fwrite(&pkt->device_id, 1, 4, file);
    }

    fclose(file);

    return 0;
}

void* mmap_seq_buf_file(char* filename, size_t max_pkt_count){
    void *mem;
    size_t mem_len;
    int fd = open(filename, O_RDONLY); 
    if(fd < 0){
         DEBUG("%s open failed\n", __FUNCTION__);
         exit(-1);
    }
    mem_len = sizeof(pkt_record) + sizeof(BLE_pkt) * max_pkt_count;
    mem = mmap(NULL, mem_len, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);

    close(fd);
    return mem;
}

int munmap_seq_buf_file(void *mem, size_t max_pkt_count){
    size_t mem_len;
    mem_len =         
    mem_len = sizeof(pkt_record) + sizeof(BLE_pkt) * max_pkt_count;

    return munmap(mem, mem_len);
}

int save_seq_buf_to_file2(pkt_record * pkt_rec, char* filename){
    uint64_t tmp;
    BLE_pkt* pkt;
    FILE *file = fopen(filename, "w+"); 
    int i;
    if(file == NULL){
         DEBUG("%s fopen failed\n", __FUNCTION__);;
         exit(-1);
    }
    fwrite(pkt_rec, sizeof(pkt_record) + sizeof(BLE_pkt) * pkt_rec->count, 1, file);  

    fclose(file);

    return 0;
}

int destroy_pkt_seq_buf(pkt_record * pkt_rec, size_t n){
    return munmap((void*)pkt_rec, sizeof(pkt_record) + n * sizeof(BLE_pkt));
}
