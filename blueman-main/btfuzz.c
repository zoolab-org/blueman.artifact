#define _GNU_SOURCE
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sched.h>
#include <stdbool.h>

#include "debug.h"
#include "btfuzz.h"
#include "pkt_record.h"
#include "utils.h"
#include "mutator/mutator.h"
#include "coverage.h"

#ifdef BLUEMAN_STAT
#include "coverage_logger/coverage_logger.h"
#endif

#define TIMES_PER_ROUND 10

// paths
static char * bsim_path;
static char * target_path;
static char * attacker_path;
static char * bsim_log_path;
static char * target_log_path;
static char * attacker_log_path;
static char * output_path;
static char * output_attack_err_log_path;
static char * output_attack_err_corpus_path;
static char * output_crash_path;
static char * output_crash_log_path;
static char * output_timeout_path;
static char * output_timeout_log_path;
static char * output_seed_path;
static char * output_core_dump_path;

// file descriptor
int bsim_log_fd, attacker_log_fd, target_log_fd;

// paket recorder
static pkt_record *pkt_rec;
static pkt_record *corpus;

// fuzzing information
static uint64_t round = 0, total_exec_times = 0, crash_count = 0;
static pid_t bsim_pid, attacker_pid, target_pid, interceptor_pid;
static uint64_t attack_error_count = 0;
static uint64_t timeout_count = 0;

// ipc between fuzzer and bsim
int qid;

// coverage
uint32_t cur_coverage = 0;
uint32_t max_total_coverage = 0;
uint32_t total_coverage = 0;

// time (in microsecond)
uint64_t start_time = 0;
uint64_t cur_time = 0;
uint64_t prev_round_time = 0;
uint64_t total_elapse = 0;
uint64_t round_elapse = 0;
struct timeval tmp_time;

// interceptor
char *interceptor_stack = NULL;
int packet_count = 0;
uint64_t total_packet_count = 0;

// selection strategy parameters
uint32_t loop_entry_index = 0;
uint32_t max_delta_index = 0;
bool mixed_mode = false;

static int copy_ble_pkt(BLE_pkt* dst, BLE_pkt* src){
    if(src->mutated == 1){
        dst->len = src->len > (dst->max_payload_len + 5) ? dst->max_payload_len + 5 : src->len;
        dst->mutated = 1;
        memcpy(dst->pkt, src->pkt, dst->len);
        return 1;
    }
    return 0;
}

int intercept_ble_packet_for_init_corpus(uint32_t max_num_of_pkt, FILE* out_file){
    int n, cnt = 0;
    msg_fuzz_pkt msg_pkt;
    msg_fuzz_event msg_event;
    uint32_t coverage_count = 0, new_coverage_count;
    char *init_corpus_file;

    uint32_t max_delta = 0;

    while(1){
        n = msgrcv(qid, &msg_pkt, sizeof(msg_pkt.pkt), MSG_FUZZ_RECV_PKT , 0);
        if(n >= 0){
            if(msg_pkt.pkt.device_id == 1){
                add_pkt_to_seq_buf(pkt_rec, &msg_pkt.pkt);
                new_coverage_count = corpus_get_coverage();

                // for PktselMode: COVERAGE_DELTA
                uint32_t delta = (new_coverage_count - coverage_count) ? 
                    (new_coverage_count - coverage_count) : 0;
                if(delta > max_delta){
                    max_delta = delta;
                    max_delta_index = pkt_rec->count - 1;
                }

            }

            do{
                n = recv_ble_pkt_ack(&msg_pkt.pkt);
            }while(n < 0);

            if(max_num_of_pkt  <= pkt_rec->count || (msg_pkt.pkt.device_id == 1 && new_coverage_count <= coverage_count && ++cnt > 80)){
                break;
            }else if(msg_pkt.pkt.device_id == 1){
                coverage_count = new_coverage_count;
            }
        }else{
            perror("");
            while(1);
        }
    }
    n = msgrcv(qid, &msg_pkt, sizeof(msg_pkt.pkt), MSG_FUZZ_RECV_PKT , 0);
    if(n >= 0){
        msg_pkt.pkt.mutated = 2;
        do{
            n = recv_ble_pkt_ack(&msg_pkt.pkt);
        }while(n < 0);
    }else{
        perror("");
        while(1);
    }
 
    init_corpus_file = str_concat(output_seed_path,  "init_corpus");
    save_seq_buf_to_file2(pkt_rec, init_corpus_file);
    free(init_corpus_file);

#ifdef PKTSEL
    if(DEFAULT_PKTSEL_MODE == MIXED_PROB){
        mixed_mode = true;
    }
    selection_strategy_setup(DEFAULT_PKTSEL_MODE, max_delta_index);
#endif
    return pkt_rec->count; 
}

void timeouthandler(int sig){
    if(kill(target_pid, SIGKILL) == -1){
        DEBUG("%s kill target failed\n", __FUNCTION__);
    }
    exit(0);
}

int intercept_ble_packet_for_fuzz(uint32_t max_num_of_pkt, FILE* out_file){ 
    int n, i;
    msg_fuzz_pkt msg_pkt;
    msg_fuzz_event msg_event;
    uint32_t coverage_count = 0, tmp_coverage_count;
    char *init_corpus_file;

    signal(SIGALRM, timeouthandler);
    alarm(2);
    for(i = 0 ; i < corpus->count ;){
        n = msgrcv(qid, &msg_pkt, sizeof(msg_pkt.pkt), MSG_FUZZ_RECV_PKT , 0);
        if(n >= 0){
            if(msg_pkt.pkt.device_id == 1){
                if(corpus->pkts_buf[i].device_id){
                    copy_ble_pkt(&msg_pkt.pkt, &corpus->pkts_buf[i]);
                    i++;
                }else{
                    DEBUG("%s error\n", __FUNCTION__);
                    while(1);
                }
                mutate_pkt(&msg_pkt.pkt, i);
                add_pkt_to_seq_buf(pkt_rec, &msg_pkt.pkt);
                packet_count = pkt_rec->count;
            }
                
            do{
                n = recv_ble_pkt_ack(&msg_pkt.pkt);
            }while(n < 0);
        }else{
            DEBUG("%s error\n", __FUNCTION__);
            while(1);
        }
    }

    n = msgrcv(qid, &msg_pkt, sizeof(msg_pkt.pkt), MSG_FUZZ_RECV_PKT , 0);
    if(n >= 0){
        do{
            n = recv_ble_pkt_ack(&msg_pkt.pkt);
        }while(n < 0);
    }else{
        perror("");
        while(1);
    }
    
    n = msgrcv(qid, &msg_pkt, sizeof(msg_pkt.pkt), MSG_FUZZ_RECV_PKT , 0);
    if(n >= 0){
        msg_pkt.pkt.mutated = 2;
        do{
            n = recv_ble_pkt_ack(&msg_pkt.pkt);
        }while(n < 0);
    }else{
        perror("");
        while(1);
    }
    return i; 
}

static int _run_ble_interceptor(void* arg){
    // child process
    int *data = arg;
    int ret;
    int max_num_of_pkt = data[0];
    int create_init_corpus = data[1];

    signal(SIGINT, SIG_IGN);
    signal(SIGKILL, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);

    if(create_init_corpus){
        ret = intercept_ble_packet_for_init_corpus(max_num_of_pkt, NULL);

    }else{
        ret = intercept_ble_packet_for_fuzz(max_num_of_pkt, NULL);
    }
    exit(0);
}
int arg[3];
int run_ble_interceptor(int max_num_of_pkt, int create_init_corpus){
    int ret;
    pid_t child_pid;
    arg[0] = max_num_of_pkt;
    arg[1] = create_init_corpus;
    arg[2] = 0;

    child_pid = clone(_run_ble_interceptor, 
                      interceptor_stack + INTERCEPTOR_STACK_SIZE, 
                      CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD, 
                      arg);

    if(child_pid > 0){
        return child_pid; 
    }else{
        DEBUG("%s fork failed\n", __FUNCTION__);
        exit(-1);
    }
}

void save_timeout_error_info(char* log_name){
    int attack_log_fd, log_fd;
    char path_buf[1024];
    off_t file_len;

    // save error corpus
    snprintf(path_buf, sizeof(path_buf), "%s/corpus_%lu", output_timeout_path, total_exec_times);
    save_seq_buf_to_file2(pkt_rec, path_buf);


    // save error log
    if(strcmp(log_name, "/dev/null")){
        snprintf(path_buf, sizeof(path_buf), "%s/log_%lu", output_timeout_log_path, total_exec_times);
        attack_log_fd = open(path_buf, O_RDWR | O_CREAT, 0600);
        if(attack_log_fd < 0){
            DEBUG("%s open failed\n", __FUNCTION__);
        }
        log_fd = open(log_name, O_RDONLY);
        if(log_fd < 0){
            DEBUG("%s open failed\n", __FUNCTION__);
        }
        file_len = lseek(log_fd, 0, SEEK_END);
        
        lseek(log_fd, 0, SEEK_SET);
        if(my_copy_file_range(log_fd, NULL, attack_log_fd, NULL, file_len, 0) < 0){
            DEBUG("%s my_copy_file_range failed\n", __FUNCTION__);
            perror("");
        }
        close(log_fd);
        close(attack_log_fd);
    }
    timeout_count++;
}

void save_signal_error_info(char* log_name, int sig, int ret){
    int attack_log_fd, log_fd;
    char path_buf[1024];
    off_t file_len;

    // save error corpus
    snprintf(path_buf, sizeof(path_buf), "%s/corpus_%lu_%d_%d", output_crash_path, total_exec_times, sig, ret);
    save_seq_buf_to_file2(pkt_rec, path_buf);


    // save error log
    if(strcmp(log_name, "/dev/null")){
        snprintf(path_buf, sizeof(path_buf), "%s/log_%lu_%d_%d", output_crash_log_path, total_exec_times, sig, ret);
        attack_log_fd = open(path_buf, O_RDWR | O_CREAT, 0600);
        if(attack_log_fd < 0){
            DEBUG("%s open failed\n", __FUNCTION__);
        }
        log_fd = open(log_name, O_RDONLY);
        if(log_fd < 0){
            DEBUG("%s open failed\n", __FUNCTION__);
        }
        file_len = lseek(log_fd, 0, SEEK_END);
        
        lseek(log_fd, 0, SEEK_SET);
        if(my_copy_file_range(log_fd, NULL, attack_log_fd, NULL, file_len, 0) < 0){
            DEBUG("%s my_copy_file_range failed\n", __FUNCTION__);
            perror("");
        }
        close(log_fd);
        close(attack_log_fd);
    }
    crash_count++;
}

void save_attack_error_info(char* log_name, int who, int sig, int ret){
    int attack_log_fd, log_fd;
    char path_buf[1024];
    off_t file_len;

    // save error corpus
    snprintf(path_buf, sizeof(path_buf), "%s/corpus_%lu_%d_%d_%d", output_attack_err_corpus_path, attack_error_count, who, sig, ret);
    save_seq_buf_to_file2(pkt_rec, path_buf);


    // save error log
    if(strcmp(log_name, "/dev/null")){
        snprintf(path_buf, sizeof(path_buf), "%s/log_%lu_%d_%d_%d", output_attack_err_log_path, attack_error_count, who, sig, ret);
        attack_log_fd = open(path_buf, O_RDWR | O_CREAT, 0600);
        if(attack_log_fd < 0){
            DEBUG("%s open failed\n", __FUNCTION__);
        }
        log_fd = open(log_name, O_RDONLY);
        if(log_fd < 0){
            DEBUG("%s open failed\n", __FUNCTION__);
        }
        file_len = lseek(log_fd, 0, SEEK_END);
        
        lseek(log_fd, 0, SEEK_SET);
        if(my_copy_file_range(log_fd, NULL, attack_log_fd, NULL, file_len, 0) < 0){
            DEBUG("%s my_copy_file_range failed\n", __FUNCTION__);
            perror("");
            while(1);
        }
        close(log_fd);
        close(attack_log_fd);
    }

    attack_error_count++;
}

static int wait_for_target(pid_t tar_pid){
    int status;
    if(waitpid(tar_pid, &status, 0) <= 0){
        DEBUG("%s waitpid failed\n", __FUNCTION__);
        exit(-1);
    }
    if(WIFSIGNALED(status)){
        // detect signaled (maybe crash)
        if(WTERMSIG(status) == SIGTRAP){
            return FAULT_ERROR;
        }else if(WTERMSIG(status) == SIGKILL){
            return FAULT_TIMEOUT;
        }else{
            save_signal_error_info(target_log_path, WTERMSIG(status), WEXITSTATUS(status));
            return FAULT_CRASH;
        }
    }
    if(WEXITSTATUS(status) != 0){
        if(WEXITSTATUS(status) == 1){
            // for asan
            save_signal_error_info(target_log_path, WTERMSIG(status), WEXITSTATUS(status));
            return FAULT_CRASH;
        }else{
            save_attack_error_info(target_log_path, 1, WTERMSIG(status), WEXITSTATUS(status));
            return FAULT_ERROR;
        }
    }

    return FAULT_NONE;
}

static pid_t run_ble_target(char** argv){
    int old_stderr_fd, ret;
    pid_t child_pid;

    child_pid = fork();
        if(!child_pid){
        // child process
        signal(SIGINT, SIG_IGN);
        old_stderr_fd = dup(STDERR_FILENO); 
        if(old_stderr_fd == -1){
            DEBUG("%s dup failed\n", __FUNCTION__);
            exit(-1);
        }
        
        ret = dup2(target_log_fd, STDERR_FILENO);
        if(ret == -1){
            DEBUG("%s dup2 failed\n", __FUNCTION__);
            exit(-1);
        }
        ret = dup2(target_log_fd, STDOUT_FILENO);
        if(ret == -1){
            DEBUG("%s dup2 failed\n", __FUNCTION__);
            exit(-1);
        }
        // set afl share memory id
        setenv(SHM_ENV_VAR, get_shm_str(), 1);
        execv(target_path, argv);
        // should not execute to here
        ret = dup2(old_stderr_fd, STDERR_FILENO);
        if(ret == -1){
            DEBUG("%s dup2 failed\n", __FUNCTION__);
        }
        DEBUG("%s execv failed\n", __FUNCTION__);
        exit(-1);
    }else if(child_pid > 0){
        // parent
        return child_pid;
    }else{
        DEBUG("%s fork failed\n", __FUNCTION__);
        exit(-1);
    }
}

static pid_t run_ble_attacker(char** argv){
    pid_t child_pid;
    int old_stderr_fd, ret;

    child_pid = fork();
    if(!child_pid){
        // child process
        signal(SIGINT, SIG_IGN);
        old_stderr_fd = dup(STDERR_FILENO); 
        if(old_stderr_fd == -1){
            DEBUG("%s dup failed\n", __FUNCTION__);
            exit(-1);
        }
        
        ret = dup2(attacker_log_fd, STDERR_FILENO);
        if(ret == -1){
            DEBUG("%s dup2 failed\n", __FUNCTION__);
            exit(-1);
        }
        ret = dup2(attacker_log_fd, STDOUT_FILENO);
        if(ret == -1){
            DEBUG("%s dup2 failed\n", __FUNCTION__);
            exit(-1);
        }

        execv(attacker_path, argv);
        // should not execute to here
        ret = dup2(old_stderr_fd, STDERR_FILENO);
        if(ret == -1){
            DEBUG("%s dup2 failed\n", __FUNCTION__);
        }
        DEBUG("%s execv failed\n", __FUNCTION__);
        exit(-1);
    }else if(child_pid > 0){
        // parent
        return child_pid;
    }else{
        DEBUG("%s fork failed\n", __FUNCTION__);
        exit(-1);
    }
}

static pid_t run_bsim_phy(char** argv){
    pid_t child_pid;
    int old_stderr_fd, ret;

    child_pid = fork();
    if(!child_pid){
        // child process
        signal(SIGINT, SIG_IGN);
        old_stderr_fd = dup(STDERR_FILENO); 
        if(old_stderr_fd == -1){
            DEBUG("%s dup failed\n", __FUNCTION__);
            exit(-1);
        }
        
        ret = dup2(bsim_log_fd, STDERR_FILENO);
        if(ret == -1){
            DEBUG("%s dup2 failed\n", __FUNCTION__);
            exit(-1);
        }
        ret = dup2(bsim_log_fd, STDOUT_FILENO);
        if(ret == -1){
            DEBUG("%s dup2 failed\n", __FUNCTION__);
            exit(-1);
        }

        execv(bsim_path, argv);
        // should not execute to here
        ret = dup2(old_stderr_fd, STDERR_FILENO);
        if(ret == -1){
            DEBUG("%s dup2 failed\n", __FUNCTION__);
        }
        DEBUG("%s execv failed\n", __FUNCTION__);
        exit(-1);
    }else if(child_pid > 0){
        // parent
        return child_pid;
    }else{
        DEBUG("%s fork failed\n", __FUNCTION__);
        exit(-1);
    }
}


int fuzz_one(int create_init_corpus){
    uint32_t fault;
    int status;
    int keeping;

    int fault_count = 0;
    char *bsim_argv[] = {"bs_2G4_phy_v1", "-nodump", "-s=trial_sim", "-D=2", "-argschannel", "-at=40", NULL};
    char *target_argv[] = {"zephyr.elf", "-s=trial_sim", "-d=1", NULL};
    char *attacker_argv[] = {"zephyr.elf", "-s=trial_sim", "-d=0", NULL};
    // reset log 
    lseek(bsim_log_fd, 0, SEEK_SET);
    lseek(attacker_log_fd, 0, SEEK_SET);
    lseek(target_log_fd, 0, SEEK_SET);
    // reset bsim
    rmrf("/tmp/bs_xiaobye");
    //system("rm -rf /tmp/bs_xiaobye");

    bsim_pid = attacker_pid = target_pid = interceptor_pid = -1;
    bsim_pid = run_bsim_phy(bsim_argv);

    target_pid = run_ble_target(target_argv);
    attacker_pid = run_ble_attacker(attacker_argv);

    interceptor_pid = run_ble_interceptor(MAX_PACKET_COUNT_PER_ITER, create_init_corpus);

    fault = wait_for_target(target_pid);

    // Clean fuzzing environment
    if(kill(bsim_pid, SIGKILL) == -1){
        DEBUG("%s kill bsim failed\n", __FUNCTION__);
    }
    waitpid(bsim_pid, NULL, 0);


    if(kill(attacker_pid, SIGKILL) == -1){
        DEBUG("%s kill attacker failed\n", __FUNCTION__);
    }
    waitpid(attacker_pid, &status, 0);

    if((WEXITSTATUS(status) != 0 ||  (WIFSIGNALED(status) && WTERMSIG(status) == SIGTRAP)) || fault == FAULT_ERROR){
        // detect attack error
        if(kill(interceptor_pid, SIGKILL) == -1){
            DEBUG("%s kill interceptor failed\n", __FUNCTION__);
        }
        delete_msg_queue();
        // setup message queue between fuzzer and bsim
        qid = create_msg_queue(); 

        if(WEXITSTATUS(status) != 0 || (WIFSIGNALED(status) && WTERMSIG(status) == SIGTRAP)){
            save_attack_error_info(attacker_log_path, 0, WTERMSIG(status), WEXITSTATUS(status));
        }
    }

    if(fault == FAULT_CRASH){
        if(kill(interceptor_pid, SIGKILL) == -1){
            DEBUG("%s kill interceptor failed\n", __FUNCTION__);
        }
        fault_count++;
        delete_msg_queue();
        // setup message queue between fuzzer and bsim
        qid = create_msg_queue();
    }

    if(fault == FAULT_TIMEOUT){
        if(!(WIFSIGNALED(status) && WTERMSIG(status) == SIGTRAP)){
            save_timeout_error_info(target_log_path);
        }
    }
    
    waitpid(interceptor_pid, NULL, 0);
    
    if(fault_count == 0 && !create_init_corpus){
        // save if interesting
        keeping = save_if_interesting(output_seed_path, pkt_rec, total_exec_times);
    }


    return fault_count;
}

static void check_crash_handling(void){
    int fd;
    char fchar;
    fd = open("/proc/sys/kernel/core_pattern", O_RDONLY);
    if(read(fd, &fchar, 1) == 1 && fchar == '|'){
        DEBUG("Please execute \"echo core >/proc/sys/kernel/core_pattern\" in you shell\n");
        exit(-1);
    }
}

static void cleanup_fuzzer_env(){
    if(bsim_pid != -1){
        if(kill(bsim_pid, SIGKILL) == 0){
            waitpid(bsim_pid,  NULL, 0);
        }
    }
    if(attacker_pid != -1){
        if(kill(attacker_pid, SIGKILL) == 0){
            waitpid(attacker_pid,  NULL, 0);
        }
    }
    
    if(interceptor_pid != -1){
        if(kill(interceptor_pid, SIGKILL) == 0){
            waitpid(interceptor_pid,  NULL, 0);
        }
    }

    if(get_shm_id() != -1){
        printf("Clean up shared memory.....\n");
        shmctl(get_shm_id(), IPC_RMID, NULL);
    }

    printf("Clean up message queue.....\n");
    if(delete_msg_queue() == -1){
        printf("Clean up message queue failed.....\n");
    }

    printf("Clean up stack of interceptor.....\n");
    if(munmap(interceptor_stack, INTERCEPTOR_STACK_SIZE) == -1){
        printf("Clean up stack of interceptor.....\n");
    }
}

void sig_handler(int sig){
    printf("signal: %d\n", sig);

    printf("Clean up fuzzer environmenet.....\n");
    cleanup_fuzzer_env();
    exit(EXIT_SUCCESS);
}

void setup_sighandler(){
    signal(SIGINT, sig_handler);
    signal(SIGSEGV, sig_handler);
    signal(SIGQUIT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGKILL, sig_handler);
}

int get_num_of_init_corpus(){
    DIR* dir = opendir(output_seed_path);
    int count = 0;
    struct dirent* d;
    if (dir) {
        while((d = readdir(dir)) != NULL){
            if(strcmp(d->d_name, ".") && strcmp(d->d_name, ".."))
                count++; 
        }
        closedir(dir);
    }
    return count;
}

void collect_init_corpus(){
    if(get_num_of_init_corpus()){
        printf("has init corpus: %d\n", get_num_of_init_corpus());
    }else{
        fuzz_one(1);
    }
    load_corpus_to_queue(output_seed_path);
    printf("Length of queue: %u\n", get_queue_length());
}

int start_fuzzer(void){
    time_t begin = time(0), end;
    time_t elapsed;
    uint64_t exec_times = 0;
    struct queue_entry* queue_cur = NULL, *queue_tail = NULL;

    // collect first valid initial corpus
    collect_init_corpus();
    // start fuzzing
    gettimeofday(&tmp_time, NULL);
    cur_time = start_time = tmp_time.tv_sec * 1000000ULL + tmp_time.tv_usec;
    while(1){
        queue_cur = get_queue_head();
        queue_tail = get_queue_tail();
        while(1){
            round++;
            prev_round_time = cur_time;
            for(int i = 0 ; i < TIMES_PER_ROUND ; i++){
                total_exec_times++;

#ifdef PKTSEL
                if (mixed_mode && total_exec_times % 2000 == 0) {
                    int mode_id = rand() % 8;  // 0 to  8 mode
                    PktselMode mode = (PktselMode)mode_id;
                    selection_strategy_setup(mode, max_delta_index);
                }
#endif

                // reset packet recorder
                init_pkt_seq_buf(pkt_rec);
                // reset trace bits map
                clean_trace_bits();
                // set corpus
                corpus = mmap_seq_buf_file(queue_cur->fname, MAX_PACKET_COUNT_PER_ITER);
                
                fuzz_one(0);
				cur_coverage = corpus_get_coverage();
                total_packet_count += packet_count;
				// update max coverage count
                total_coverage = get_total_coverage();
				if(total_coverage > max_total_coverage) max_total_coverage = total_coverage;


                munmap_seq_buf_file(corpus, MAX_PACKET_COUNT_PER_ITER);
                gettimeofday(&tmp_time, NULL);
                cur_time = tmp_time.tv_sec * 1000000ULL + tmp_time.tv_usec;
            }
            round_elapse = cur_time - prev_round_time;
            total_elapse = cur_time - start_time;

            printf("start_time: %lu, cur_time: %lu, total_elapse: %lu, round_elapse: %lu\n", \
                   start_time, \
                   cur_time, \
                   total_elapse, \
                   round_elapse
            );
            printf("round %lu, exec_counts %lu, current corpus: %s, attack_error_count %lu, crash_count %lu, timeout_count: %lu, coverage: %u, max coverage: %u, queue size: %u, total_packet_count: %lu\n",\
                    round,\
                    total_exec_times,
                    queue_cur->fname,\
                    attack_error_count,\
                    crash_count,\
                    timeout_count,\
                    cur_coverage,\
                    max_total_coverage,\
                    get_queue_size(),\
                    total_packet_count);

#ifdef BLUEMAN_STAT
	    add_coverage_entry(total_elapse, max_total_coverage);
#ifdef TIMER
	    if(total_elapse > TIMER * 1000000ULL){
                save_coverage_to_file();
                printf("Save coverage to file.....\n");
                cleanup_fuzzer_env();
                exit(EXIT_SUCCESS);
	    }
#endif
#endif

            if(queue_cur == queue_tail) break;
            queue_cur = get_queue_next(queue_cur);
        }
    }
    end = time(0);
    elapsed = end - begin;
    printf("The elapsed time is %lu seconds\n", elapsed);
}

int init_fuzzer(char* bsim_p, char* attacker_p, char* target_p, char* inst_id){
    struct stat st = {0};
    setbuf(stdout, NULL);
    setbuf(stderr,  NULL);

    bsim_path = bsim_p;
    target_path = target_p;
    attacker_path = attacker_p;

    bsim_log_path = "/tmp/bsim_log";
    attacker_log_path = "/tmp/attacker_log";
    target_log_path = "/tmp/target_log";

    output_path = OUTPUT_PATH"/";
    output_crash_path = OUTPUT_PATH"/crash/";
    output_crash_log_path = OUTPUT_PATH"/crash_log/";
    output_timeout_path = OUTPUT_PATH"/timeout/";
    output_timeout_log_path = OUTPUT_PATH"/timeout_log/";
    output_seed_path = OUTPUT_PATH"/seed/";
    output_core_dump_path = OUTPUT_PATH"/core_dump/";
    output_attack_err_log_path = OUTPUT_PATH"/attack_err_log/";
    output_attack_err_corpus_path = OUTPUT_PATH"/attack_err_corpus/";
    // file descriptor
    bsim_log_fd = open(bsim_log_path, O_RDWR | O_CREAT | O_TRUNC, 0600);
    attacker_log_fd = open(attacker_log_path, O_RDWR | O_CREAT | O_TRUNC, 0600);
    target_log_fd = open(target_log_path, O_RDWR | O_CREAT | O_TRUNC, 0600);

    // paket recorder
    pkt_rec = create_pkt_seq_buf(MAX_PACKET_COUNT_PER_ITER * 2);

    // creaate output directory
    if (stat(output_path, &st) == -1) {
        mkdir(output_path, 0700);
    }
    if (stat(output_crash_path, &st) == -1) {
        mkdir(output_crash_path, 0700);
    }
    if (stat(output_crash_log_path, &st) == -1) {
        mkdir(output_crash_log_path, 0700);
    }
    if (stat(output_timeout_path, &st) == -1) {
        mkdir(output_timeout_path, 0700);
    }
    if (stat(output_timeout_log_path, &st) == -1) {
        mkdir(output_timeout_log_path, 0700);
    }
    if (stat(output_seed_path, &st) == -1) {
        mkdir(output_seed_path, 0700);
    }
    if (stat(output_core_dump_path, &st) == -1) {
        mkdir(output_core_dump_path, 0700);
    }
    if (stat(output_attack_err_log_path, &st) == -1) {
        mkdir(output_attack_err_log_path, 0700);
    }
    if (stat(output_attack_err_corpus_path, &st) == -1) {
        mkdir(output_attack_err_corpus_path, 0700);
    }
    // setup shared memory 
    setup_shm();

    // setup message queue between fuzzer and bsim
    qid = create_msg_queue();  

    // setup signal handler
    setup_sighandler();

    // interceptor stack
    interceptor_stack = mmap(NULL, INTERCEPTOR_STACK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); 
    if(interceptor_stack == MAP_FAILED){
		fprintf(stderr, "Create stack of interceptor failed\n");
        return EXIT_FAILURE;
    }
#ifdef BLUEMAN_STAT
    init_coverage_logger(inst_id);
#endif
    // mutator initialization
    mutator_init();
}




