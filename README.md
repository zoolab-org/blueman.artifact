# blueman.artifact

## Setup
1. **Please refer to the [official Docker documentation](https://docs.docker.com/engine/install/) for installation instructions**
2. **Clone the repository and build the Docker image for the fuzzing environment**

    ```bash
    chmod +x ./build.sh
    chmod +x ./run.sh
    ./build.sh
    ```

---

## Test Target
Executing **./run.sh** without any arguments will display the usage information for the tool
```bash
Usage: ./run.sh <action> <mutator> [packet selection strategies] <output_dir>

Available actions:
  - gatt_write_peripheral
  - hr_peripheral
  - sm_pairing_peripheral
  - gatt_write_central
  - hr_central
  - sm_pairing_central

Available mutators:
  - field
  - afl
  - random

Available packet selections(field-aware mutator only):
  - FIXED_PROB_10
  - FIXED_PROB_25
  - FIXED_PROB_50
  - FIXED_PROB_75
  - FIXED_PROB_100
  - SELECTIVE_25_75
  - SELECTIVE_75_25
  - RANDOM_PROB
  - MIXED_PROB
```
### Available Actions
- gatt_write_peripheral
    - Testing the **samples
/bluetooth/peripheral_gatt_write** exmaple in Zephyr
- hr_peripheral
    - Testing the **samples
/bluetooth/peripheral_hr** exmaple in Zephyr
- sm_pairing_peripheral
    - Testing the **example/sm_pairing_peripheral.c** exmaple in BTstack
- gatt_write_central
    - Testing the **samples
/bluetooth/central_gatt_write** exmaple in Zephyr
- hr_central
    - Testing the **samples
/bluetooth/central_hr** exmaple in Zephyr
- sm_pairing_central
    - Testing the **example/sm_pairing_central.c** exmaple in BTstack
### Available Mutators
- field
    - The mutator understands Bluetooth protocol structure. The packet is mutated within the bounds defined by the corresponding protocol specification.
- afl
    - Apply the havoc mutators from AFL.
- random
    - Apply random bytes mutations to the entire packet.

### Available Packet selection strategies (field-aware mutator only)
- FIXED_PROB_10/ FIXED_PROB_25/ FIXED_PROB_50/ FIXED_PROB_75/ FIXED_PROB_100: Fixed mutation probabilities
- SELECTIVE_25_75/ SELECTIVE_75_25: Different probabilities before/after coverage delta points
- RANDOM_PROB: Dynamic random probabilities (10%-100%)
- MIXED_PROB: Mixed strategy mode

## Start Fuzzing
To start fuzzing, you need to choose an action and a mutator from the list. **Each new fuzz test should use a separate directory to store the results**.

For instance, if you want to test sm_pairing_peripheral with the AFL mutator, you can run the following commands:
```bash
mkdir results
./run.sh sm_pairing_peripheral afl ./results
```
Or if you want to test gatt_write_central with the field mutator, you can run the following commands:
```bash
mkdir results
./run.sh gatt_write_central field FIXED_PROB_100 ./results
```


