#define _XOPEN_SOURCE 500

#include <ftw.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/mman.h>


#include "utils.h"
char* str_uint64(uint64_t n){
    char *out_str = (char * )malloc(30);
    snprintf(out_str, 30, "%lu", n);
    return out_str;
}
char* str_concat(char* str1, char* str2){
    int total_len = strlen(str1) + strlen(str2) + 2;
    char *out_str = (char * )malloc(total_len);
    strcpy(out_str, str1);
    strcat(out_str, str2);
    return out_str;
}

void str_free(char* str){
    free(str);
}

void list_add(struct list_head *node, struct list_head *head){
    struct list_head *next = head->next;

    next->prev = node;
    node->next = next;
    node->prev = head;
    head->next = node;
}

void list_add_tail(struct list_head *node, struct list_head *head){
    struct list_head *tail = head->prev;

    tail->next = node;
    head->prev = node;
    node->next = head;
    node->prev = tail;
}

void list_del(struct list_head *node){
    struct list_head *next = node->next;
    struct list_head *prev = node->prev;

    next->prev = prev;
    prev->next = next;
}

void init_list_head(struct list_head* node){
	node->prev = node;
	node->next = node;
}

void list_del_init(struct list_head *node){
    list_del(node);
    INIT_LIST_HEAD(node);
}

int list_empty(const struct list_head *head){
    return (head->next == head);
}

uint64_t get_cur_time(void) {

  struct timeval tv;

  gettimeofday(&tv, NULL);

  return (tv.tv_sec * 1000ULL) + (tv.tv_usec / 1000);

}

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int rv = remove(fpath);

    if (rv)
        perror(fpath);

    return rv;
}

int rmrf(char *path)
{
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

int my_copy_file_range(int fd_in, void* off_in, int fd_out, void* off_out, size_t len, uint32_t flags){
    char *in_mem; 
    ssize_t n;
    in_mem = mmap(NULL, len, PROT_READ, MAP_SHARED, fd_in, 0);
    if(in_mem == (void*)-1) return -1;
   
    while(1){
        n = write(fd_out, in_mem, len); 
        if(n == -1) goto end;
        if(n < len) len -= n;
        else break;
    }

end:
    if(munmap(in_mem, len) == -1){
        return -1;
    }else{
        return 0;
    }
}
