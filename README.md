# blueman.public

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
Usage: ./run.sh <action> <mutator> <output_dir>

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


## Start Fuzzing
To start fuzzing, you need to choose an action and a mutator from the list. **Each new fuzz test should use a separate directory to store the results**.

For instance, if you want to test sm_pairing_peripheral with the AFL mutator, you can run the following commands:
```bash
mkdir results
./run.sh sm_pairing_peripheral afl ./results
```



