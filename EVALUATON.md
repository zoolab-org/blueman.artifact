# Evaluation Instructions
This document explains how to reproduce the **GATT Write (to Peripheral)** coverage experiment described in **Figure 13.** of the pape. 

We use the `gatt_write_peripheral` example to walk through all steps.

## 1. Prepare Three Separate Terminals
> [!IMPORTANT]
Let all three processes run for exactly 24 hours before proceeding.

You will need three terminals open. In each terminal, you will run one of the following commands:

1. **Terminal 1**: Run the “field” strategy
```bash
mkdir -p gatt_write_peripheral_field
./run.sh gatt_write_peripheral field FIXED_PROB_50 ./gatt_write_peripheral_field
```

2. **Terminal 2**: Run the “random” strategy
```bash
mkdir -p gatt_write_peripheral_random
./run.sh gatt_write_peripheral random ./gatt_write_peripheral_random
```

3. **Terminal 3**: Run the “AFL-like” strategy
```bash
mkdir -p gatt_write_peripheral_afl
./run.sh gatt_write_peripheral afl ./gatt_write_peripheral_afl
```

## 2. Stop the Processes and Collect Coverage Data

After 24 hours have elapsed, press `Ctrl + \` (i.e., send `SIGQUIT`) in **each** of the three terminals. Upon receiving `SIGQUIT`, each process will immediately flush its internal statistics to a file in its working directory.

Each directory will now contain a statistics file (named by strategy: `field`, `random`, or `afl`).

## 3. Copy Statistic Files to a Unified Input Directory

Create a new directory called `eval_gatt_write_peripheral/input_dir`, and copy the three statistics files into it:

```bash
mkdir -p eval_gatt_write_peripheral/input_dir

cp gatt_write_peripheral_field/field    eval_gatt_write_peripheral/input_dir
cp gatt_write_peripheral_random/random  eval_gatt_write_peripheral/input_dir
cp gatt_write_peripheral_afl/afl        eval_gatt_write_peripheral/input_dir
````

After running these commands, you should have:

```
eval_gatt_write_peripheral/
└── input_dir/
    ├── field
    ├── random
    └── afl
```

## 4. Generate the Coverage PDF

With all three strategy outputs in `eval_gatt_write_peripheral/input_dir`, run:

```bash
./gen_rq3.sh $PWD/eval_gatt_write_peripheral
```

This script will process the three statistic files and produce a PDF showing coverage curves, saving it to:

```
eval_gatt_write_peripheral/cov/coverage.pdf
```

That PDF corresponds to Figure 13 in the paper.




