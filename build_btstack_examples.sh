cd /root/btstack/port/zephyr
source /root/.zephyrrc
source env.sh

EXAMPLE=sm_pairing_peripheral west build -d build/sm_pairing_peripheral -b nrf52_bsim -- -DCONFIG_WITHOUT_BTFUZZ=y 
EXAMPLE=sm_pairing_central west build -d build/sm_pairing_central -b nrf52_bsim -- -DCONFIG_WITHOUT_BTFUZZ=y 

export AFL_CC=$(which gcc) 
export AFL_PATH=~/AFL_FOR_BTSTACK
export PATH=$HOME/bin/:$PATH

EXAMPLE=sm_pairing_central west build -d build/instrumented_sm_pairing_central -b nrf52_bsim
EXAMPLE=sm_pairing_peripheral west build -d build/instrumented_sm_pairing_peripheral -b nrf52_bsim

