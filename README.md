# BLuEMan: A Stateful Simulation-based Fuzzing Framework for Open-Source RTOS Bluetooth Low Energy Protocol Stacks

**Authors:** Wei-Che Kao, Yen-Chia Chen, Yu-Sheng Lin, Yu-Cheng Yang, Chi-Yu Li, Chun-Ying Huang

This is the repository for the artifact accompanying the paper "BLuEMan: A Stateful Simulation-based Fuzzing Framework for Open-Source RTOS Bluetooth Low Energy Protocol Stacks."

**Full Paper:** TBA

## Setup
> [!IMPORTANT]
Please note that our tool can only be run on x86_64 machines
1. **Please refer to the [official Docker documentation](https://docs.docker.com/engine/install/) for installation instructions**
2. **Clone the repository and build the Docker image for the fuzzing environment**

    ```bash
    chmod +x ./build.sh
    chmod +x ./run.sh
    ./build.sh
    ```

## Test Target
Executing **./run.sh** without any arguments displays the usage information for the tool.
```bash
Usage: ./run.sh <action> <mutator> [packet selection strategies] <execution duration> <output_dir>

Available actions:
  - gatt_write_peripheral
  - hr_peripheral
  - sm_pairing_peripheral
  - le_credit_server
  - ots_peripheral
  - gatt_write_central
  - hr_central
  - sm_pairing_central
  - le_credit_client
  - otc_central

Available mutators:
  - field
  - afl
  - random

Available packet selection strategies (for field mutator only):
  - FIXED_PROB_10
  - FIXED_PROB_25
  - FIXED_PROB_50
  - FIXED_PROB_75
  - FIXED_PROB_100
  - SELECTIVE_25_75
  - SELECTIVE_75_25
  - RANDOM_PROB
  - MIXED_PROB

Available execution duration (in minutes):
  - 1
  - 10
  - 60
  - 720
  - 1440
  - 43200
```
### Available Actions
- gatt_write_peripheral
    - Testing the **samples
/bluetooth/peripheral_gatt_write** example in Zephyr
- hr_peripheral
    - Testing the **samples
/bluetooth/peripheral_hr** example in Zephyr
- sm_pairing_peripheral
    - Testing the **example/sm_pairing_peripheral.c** example in BTstack
- le_credit_server
    - Testing the **example/le_credit_based_flow_control_mode_server.c** example in BTstack
- gatt_write_central
    - Testing the **samples
/bluetooth/central_gatt_write** example in Zephyr
- hr_central
    - Testing the **samples
/bluetooth/central_hr** example in Zephyr
- sm_pairing_central
    - Testing the **example/sm_pairing_central.c** example in BTstack
- le_credit_client
    - Testing the **example/le_credit_based_flow_control_mode_client.c** example in BTstack
### Available Mutators
- field
    - The mutator understands Bluetooth protocol structure. The packet is mutated within the bounds defined by the corresponding protocol specification.
- afl
    - Apply the havoc mutators from AFL.
- random
    - Apply random byte mutations to the entire packet.

### Available Packet selection strategies (field-aware mutator only)
- FIXED_PROB_10/ FIXED_PROB_25/ FIXED_PROB_50/ FIXED_PROB_75/ FIXED_PROB_100: Fixed mutation probabilities
- SELECTIVE_25_75/ SELECTIVE_75_25: Different probabilities before/after coverage delta points
- RANDOM_PROB: Dynamic random probabilities (10%-100%)
- MIXED_PROB: Mixed strategy mode

## Start Fuzzing
To start fuzzing, choose at least an action and a mutator from the list.
> [!IMPORTANT]
> Each new fuzz test should use a separate directory to store the results.

For instance, if you want to test sm_pairing_peripheral for 1 minute with the AFL mutator, you can run the following commands:
```bash
mkdir results
./run.sh sm_pairing_peripheral afl 1 $PWD/results
```
Or if you want to test gatt_write_central for 1 minute with the field mutator, you can run the following commands:
```bash
mkdir results
./run.sh gatt_write_central field FIXED_PROB_100 1 $PWD/results
```
## Auto Run RQ3 and RQ4
The following command runs three different mutator in RQ3 experiments — `field-aware`, `random`, and `afl` — based on the specified `action`.
```bash
./auto_gen_rq3.sh <action> <execution duration>
```
The resulting plot will be saved to:
`$PWD/<action>_rq3/eval_<action>/cov/coverage.pdf`

The following fommand runs nine different packet selection strategies in RQ4 experiments — `FIXED_PROB_10`, `FIXED_PROB_25`, `FIXED_PROB_50`, `FIXED_PROB_75`, `FIXED_PROB_100`, `SELECTIVE_25_75`, `SELECTIVE_75_25`, `RANDOM_PROB`, `MIXED_PROB` — based on the specified `action`.
```bash
./auto_gen_rq4.sh <action> <execution duration>
```
The resulting plot will be saved to:
`$PWD/<action>_rq4/eval_<action>/cov/coverage.pdf`

## Reported CVEs

- **CVE-2023-4424**: A malicious BLE device can cause buffer overflow by sending malformed advertising packet BLE device using Zephyr OS, leading to DoS or potential RCE on the victim BLE device [[issue tracking](https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-j4qm-xgpf-qjw3)].
- **CVE-2024-3077**: A malicious BLE device can crash a BLE victim device by sending malformed gatt packets [[issue tracking](https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-gmfv-4vfh-2mh8)].
- **CVE-2024-3332**: A malicious BLE device can send a specific order of packet sequence to cause a DoS attack on the victim BLE device [[issue tracking](https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-jmr9-xw2v-5vf4)].
- **CVE-2024-4785**: BT: Missing Check in LL_CONNECTION_UPDATE_IND Packet Leads to Division by Zero [[issue tracking](https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-xcr5-5g98-mchp)].

