ifndef OUTPUT_PATH
	OUTPUT_PATH = ./run/
endif

VALID_MODES := FIXED_PROB_10 FIXED_PROB_25 FIXED_PROB_50 FIXED_PROB_75 FIXED_PROB_100 \
               SELECTIVE_25_75 SELECTIVE_75_25 RANDOM_PROB MIXED_PROB
ifndef PKTSEL_MODE
	PKTSEL_FLAGS =
else
# Check if PKTSEL_MODE is valid
ifeq ($(filter $(PKTSEL_MODE),$(VALID_MODES)),)
$(error Error: Invalid PKTSEL_MODE '$(PKTSEL_MODE)'. Valid options: $(VALID_MODES))
endif
	PKTSEL = $(PKTSEL_MODE)
	PKTSEL_FLAGS = -DPKTSEL=1
endif

MUTATION_FLAGS = -DDEFAULT_PKTSEL_MODE=$(PKTSEL) $(PKTSEL_FLAGS)

FUZZER_FILES = main.c \
			   btfuzz.c \
			   coverage.c \
			   pkt_record.c \
			   interceptor.c \
			   utils.c \
			   mutator/mutate_strategy.c \

ifeq ($(MUTATOR), random)
	FUZZER_FILES += mutator/mutator_random.c
else ifeq ($(MUTATOR), afl)
	FUZZER_FILES += mutator/mutator_afl4.c
else
	FUZZER_FILES += mutator/mutator_full_stack2.c
endif

TIMER=30

ifeq ($(SET_TIMER), 86400)
	TIMER=86400
else ifeq ($(SET_TIMER), 43200)
	TIMER=43200
else ifeq ($(SET_TIMER), 21600)
	TIMER=21600
else ifeq ($(SET_TIMER), 7200)
	TIMER=7200
else ifeq ($(SET_TIMER), 3600)
	TIMER=3600
else ifeq ($(SET_TIMER), 600)
	TIMER=600
else ifeq ($(SET_TIMER), 60)
	TIMER=60
else
	TIMER=2592000
endif

STATISTICS_API=coverage_logger/coverage_logger.c

all:
	gcc $(FUZZER_FILES) $(MUTATION_FLAGS) -I$(PWD)/include -o $(OUTPUT_PATH)/main -g $(CFLAGS)
build_stat:
	gcc $(FUZZER_FILES) $(MUTATION_FLAGS) $(STATISTICS_API) $(MONGOC_FLAGS) -I$(PWD)/include -ljson-c -o $(OUTPUT_PATH)/main -g -DBLUEMAN_STAT=1 $(CFLAGS) -DTIMER=$(TIMER)
clean:
	rm $(OUTPUT_PATH)/main
