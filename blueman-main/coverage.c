#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include "debug.h"
#include "coverage.h"

uint32_t queued_paths = 0;
uint64_t last_path_time = 0;
// coverage
int shm_id = -1;
uint8_t* trace_bits = NULL;
uint8_t virgin_bits[MAP_SIZE];
char shm_str[30] = {0};
struct queue_entry* queue_top = NULL;
LIST_HEAD(queue);

int get_shm_id(void){
    return shm_id;
}

char* get_shm_str(void){
    return shm_str;
}

void setup_shm(void) {
    memset(virgin_bits, 0xff, MAP_SIZE);

    shm_id = shmget(IPC_PRIVATE, MAP_SIZE, IPC_CREAT | IPC_EXCL | 0600);
    if (shm_id < 0){
        DEBUG("%s shmget failed\n", __FUNCTION__);
        exit(-1);
    }
    sprintf(shm_str, "%d", shm_id);
    trace_bits = shmat(shm_id, NULL, 0);
    memset(trace_bits, 0x00, MAP_SIZE);
    if (trace_bits == (void *)-1){
        DEBUG("%s shmat failed\n", __FUNCTION__);
        exit(-1);
    }
}

void clean_trace_bits(void){
    memset(trace_bits, 0x00, MAP_SIZE);
}

static inline uint8_t has_new_bits(uint8_t* virgin_map){
    uint64_t* current = (uint64_t*)trace_bits;
    uint64_t* virgin  = (uint64_t*)virgin_map;
    uint8_t ret = 0;

    for(uint32_t i = 0 ; i < (MAP_SIZE >> 3) ; i++){
        if (unlikely(*current) && unlikely(*current & *virgin)) {
            if (likely(ret < 2)) {
                uint8_t* cur = (uint8_t*)current;
                uint8_t* vir = (uint8_t*)virgin;
                if ((cur[0] && vir[0] == 0xff) || (cur[1] && vir[1] == 0xff) ||
                    (cur[2] && vir[2] == 0xff) || (cur[3] && vir[3] == 0xff) ||
                    (cur[4] && vir[4] == 0xff) || (cur[5] && vir[5] == 0xff) ||
                    (cur[6] && vir[6] == 0xff) || (cur[7] && vir[7] == 0xff)) ret = 2;
                else ret = 1;
            }
            *virgin &= ~*current;

        }
        current++;
        virgin++;
    }
    return ret;
}

static uint32_t count_non_255_bytes(uint8_t* mem){

    uint32_t* ptr = (uint32_t*)mem;
    int32_t i = (MAP_SIZE >> 2);
    int32_t ret = 0;

    while (i--) {

        uint32_t v = *(ptr++);

        if (v == 0xffffffff) continue;
        if ((v & FF(0)) != FF(0)) ret++;
        if ((v & FF(1)) != FF(1)) ret++;
        if ((v & FF(2)) != FF(2)) ret++;
        if ((v & FF(3)) != FF(3)) ret++;

    }

    return ret;
}

uint32_t get_total_coverage(){
    uint32_t count = count_non_255_bytes(virgin_bits);
    return count;
}

static uint32_t count_bytes(uint8_t* mem) {
    uint64_t* ptr = (uint64_t*)mem;
    uint32_t  i   = (MAP_SIZE >> 3);
    uint32_t  ret = 0;

    while (i--) {

        uint64_t v = *(ptr++);

        if (!v) continue;
        ret += !!(v & FF(0));
        ret += !!(v & FF(1));
        ret += !!(v & FF(2));
        ret += !!(v & FF(3));
        ret += !!(v & FF(4));
        ret += !!(v & FF(5));
        ret += !!(v & FF(6));
        ret += !!(v & FF(7));
    }

    return ret;

}

static void add_to_queue(char* fname, pkt_record* pkt_rec) {

    struct queue_entry* q = malloc(sizeof(struct queue_entry));
    
    q->fname = (char*)malloc(strlen(fname) + 1); 
    strcpy(q->fname, fname);
    q->count = pkt_rec->count;
    q->len = get_pkt_seq_total_len(pkt_rec);


    if (queue_top) {
        list_add(&q->list, &queue);
        queue_top = q;
    } else {
        list_add(&q->list, &queue);
        queue_top = q;
    }

    queued_paths++;
    last_path_time = get_cur_time();
}

uint32_t corpus_get_coverage(){
    uint32_t count = count_bytes(trace_bits);
    return count;
}

uint8_t save_if_interesting(char* out_dir, pkt_record *pkt_rec,uint32_t round) {
    uint8_t  hnb;
    int fd;
    uint32_t cov_count;
    uint8_t  keeping = 0, res;
    char path_buf[300] = {0};


    if (!(hnb = has_new_bits(virgin_bits))) {
        return 0;
    }
    cov_count = corpus_get_coverage();

    snprintf(path_buf, sizeof(path_buf), "%s/id_%08u_%016u_%06u", out_dir, cov_count, round, queued_paths);

    add_to_queue(path_buf, pkt_rec);

    if (hnb == 2) {
        queue_top->has_new_cov = 1;
    }

    save_seq_buf_to_file2(pkt_rec, path_buf);

    keeping = 1;

    return keeping;
}

void load_corpus_to_queue(char* corpus_path){
    DIR* dir;
    struct dirent* d;
    char path[1000];
    pkt_record* rec;
   
    dir = opendir(corpus_path);
    if (dir) {
        while((d = readdir(dir)) != NULL){
            if(strcmp(d->d_name, ".") && strcmp(d->d_name, "..")){
                printf("Start loading %s\n", d->d_name);
                snprintf(path, sizeof(path), "%s/%s", corpus_path, d->d_name);

                rec = mmap_seq_buf_file(path, MAX_PACKET_COUNT_PER_ITER);
                add_to_queue(path, rec); 
                munmap_seq_buf_file(rec, MAX_PACKET_COUNT_PER_ITER);
            }
        }
        closedir(dir);
    }
}

void print_queue_entry_info(struct queue_entry* q){
    printf("q->fname: %s\n", q->fname);
    printf("q->count: %u\n", q->count);
    printf("q->len: %u\n\n", q->len);
}

uint32_t get_queue_length(){
    struct list_head* node;
    struct queue_entry* q;
    uint32_t ret = 0;
    list_for_each(node, &queue){
        q = list_entry(node, struct queue_entry, list);
        print_queue_entry_info(q);
        ret++; 
    }
    return ret;
}

struct queue_entry* get_queue_head(){
   return list_first_entry(&queue, struct queue_entry, list);
}

struct queue_entry* get_queue_tail(){
   return list_last_entry(&queue, struct queue_entry, list);
}

struct queue_entry* get_queue_next(struct queue_entry* q){
   return list_first_entry(&q->list, struct queue_entry, list);
}

uint32_t get_queue_size(){
	return queued_paths;
}


