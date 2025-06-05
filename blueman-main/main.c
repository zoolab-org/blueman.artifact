#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

#include "btfuzz.h"

int main(int argc, char** argv){
    if(argc > 4){
        init_fuzzer(argv[1], argv[2], argv[3], argv[4]);
    }else{
        printf("./btfuzz <bsim> <BLE attacker> <BLE victim> <instance_id>\n");
        exit(0);
    }
    start_fuzzer(); 
    return 0;
}

