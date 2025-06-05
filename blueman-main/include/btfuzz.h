#ifndef __BTFUZZ_H_
#define __BTFUZZ_H_
#include <stdio.h>
#include "interceptor.h"
#include "config.h"

int intercept_ble_packet(uint32_t, FILE*);
int fuzz_one();
int start_fuzzer(void);
int init_fuzzer(char*, char* ,char*, char* inst_id);

#define FAULT_CRASH 0xdeadbeaf
#define FAULT_ERROR 0xeeeeffff
#define FAULT_TIMEOUT 0xeeefffff
#define FAULT_NONE 0
#define INTERCEPTOR_STACK_SIZE (8192 * 5)

#endif
