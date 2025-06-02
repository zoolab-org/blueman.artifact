cd /root/zephyrproject/zephyr 
source /root/zephyrproject/.venv/bin/activate 
source /root/zephyrproject/zephyr/zephyr-env.sh 
source /root/.zephyrrc 
west build -b nrf52_bsim \
  -d build/central_hr \
  samples/bluetooth/central_hr \
  -- -DCONFIG_WITHOUT_BTFUZZ=y

west build -b nrf52_bsim \
  -d build/peripheral \
  samples/bluetooth/peripheral \
  -- -DCONFIG_WITHOUT_BTFUZZ=y

west build -b nrf52_bsim \
  -d build/peripheral_gatt_write \
  samples/bluetooth/peripheral_gatt_write \
  -- -DCONFIG_WITHOUT_BTFUZZ=y

west build -b nrf52_bsim \
  -d build/central_gatt_write \
  samples/bluetooth/central_gatt_write \
  -- -DCONFIG_WITHOUT_BTFUZZ=y

west build -b nrf52_bsim \
  -d build/central_otc \
  samples/bluetooth/central_otc \
  -- -DCONFIG_WITHOUT_BTFUZZ=y

west build -b nrf52_bsim \
  -d build/peripheral_ots \
  samples/bluetooth/peripheral_ots \
  -- -DCONFIG_WITHOUT_BTFUZZ=y

export AFL_CC=$(which gcc) 
export AFL_PATH=~/AFL_FOR_ZEPHYR_BLE 
export PATH=$HOME/bin/:$PATH
west build -b nrf52_bsim \
  -d build/instrumented_peripheral \
  samples/bluetooth/peripheral

west build -b nrf52_bsim \
  -d build/instrumented_central_hr \
  samples/bluetooth/central_hr

west build -b nrf52_bsim \
  -d build/instrumented_central_gatt_write \
  samples/bluetooth/central_gatt_write

west build -b nrf52_bsim \
  -d build/instrumented_peripheral_gatt_write \
  samples/bluetooth/peripheral_gatt_write

west build -b nrf52_bsim \
  -d build/instrumented_central_otc \
  samples/bluetooth/central_otc

west build -b nrf52_bsim \
  -d build/instrumented_peripheral_ots \
  samples/bluetooth/peripheral_ots

