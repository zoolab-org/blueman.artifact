#ifndef __UTILS_H_
#define __UTILS_H_

#include <stdint.h>
#include <stddef.h>

uint64_t get_cur_time(void);
char* str_uint64(uint64_t);
char* str_concat(char * , char * );
void str_free(char * );

struct list_head{
    struct list_head* prev, *next;  
};


/* Doubly linked list */
#define container_of(ptr, type, member) \
    ((type *) ((char *) (ptr) - offsetof(type, member)))

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(name) init_list_head(name)

#define list_entry(node, type, member) container_of(node, type, member)

#define list_first_entry(head, type, member) \
    list_entry((head)->next, type, member)

#define list_last_entry(head, type, member) \
    list_entry((head)->prev, type, member)

#define list_for_each(node, head) \
    for (node = (head)->next; node != (head); node = node->next)

#define list_for_each_safe(node, safe, head)                     \
    for (node = (head)->next, safe = node->next; node != (head); \
         node = safe, safe = node->next)

void list_add(struct list_head*, struct list_head*);
void init_list_head(struct list_head*);
void list_add_tail(struct list_head*, struct list_head*);
void list_del(struct list_head*);
void list_del_init(struct list_head*);
int list_empty(const struct list_head*);
int rmrf(char *);
int my_copy_file_range(int, void*, int, void*, size_t, uint32_t);

#define likely(_x)   __builtin_expect(!!(_x), 1)
#define unlikely(_x)  __builtin_expect(!!(_x), 0)
#define ARRAY_SIZE(arr)    (sizeof(arr) / sizeof((arr)[0]))

#endif


