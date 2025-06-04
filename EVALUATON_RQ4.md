# Evaluation Instructions
This document explains how to reproduce the **OTS(to Peripheral)** coverage experiment described in **Figure 14.** of the pape. 

We use the `peripheral_ots` example to walk through all steps.

## 1. Prepare Nine Separate Terminals
> [!IMPORTANT]
The following runs FIXED_PROB_10, FIXED_PROB_25, FIXED_PROB_50, FIXED_PROB_75, FIXED_PROB_100, SELECTIVE_25_75, SELECTIVE_75_25, RANDOM_PROB, MIXED_PROB for only 10 minutes each. To run longer tests, please modify the execution duration parameter accordingly.

You will need nine terminals open. In each terminal, you will run one of the following commands:

1. **Terminal 1**: Run the “FIXED_PROB_10” strategy
```bash
mkdir -p peripheral_ots_FIXED_PROB_10
./run.sh peripheral_ots field FIXED_PROB_10 10 $PWD/peripheral_ots_FIXED_PROB_10
```

2. **Terminal 2**: Run the “FIXED_PROB_25” strategy
```bash
mkdir -p peripheral_ots_FIXED_PROB_25
./run.sh peripheral_ots field FIXED_PROB_25 10 $PWD/peripheral_ots_FIXED_PROB_25
```

3. **Terminal 3**: Run the “FIXED_PROB_50” strategy
```bash
mkdir -p peripheral_ots_FIXED_PROB_50
./run.sh peripheral_ots field FIXED_PROB_50 10 $PWD/peripheral_ots_FIXED_PROB_50
```

4. **Terminal 4**: Run the “FIXED_PROB_75” strategy
```bash
mkdir -p peripheral_ots_FIXED_PROB_75
./run.sh peripheral_ots field FIXED_PROB_75 10 $PWD/peripheral_ots_FIXED_PROB_75
```

5. **Terminal 5**: Run the “FIXED_PROB_100” strategy
```bash
mkdir -p peripheral_ots_FIXED_PROB_100
./run.sh peripheral_ots field FIXED_PROB_100 10 $PWD/peripheral_ots_FIXED_PROB_100
```

6. **Terminal 6**: Run the “SELECTIVE_25_75” strategy
```bash
mkdir -p peripheral_ots_SELECTIVE_25_75
./run.sh peripheral_ots field SELECTIVE_25_75 10 $PWD/peripheral_ots_SELECTIVE_25_75
```

7. **Terminal 7**: Run the “SELECTIVE_75_25” strategy
```bash
mkdir -p peripheral_ots_SELECTIVE_75_25
./run.sh peripheral_ots field SELECTIVE_75_25 10 $PWD/peripheral_ots_SELECTIVE_75_25
```

8. **Terminal 8**: Run the “RANDOM_PROB” strategy
```bash
mkdir -p peripheral_ots_RANDOM_PROB
./run.sh peripheral_ots field RANDOM_PROB 10 $PWD/peripheral_ots_RANDOM_PROB
```

9. **Terminal 9**: Run the “MIXED_PROB” strategy
```bash
mkdir -p peripheral_ots_MIXED_PROB
./run.sh peripheral_ots field MIXED_PROB 10 $PWD/peripheral_ots_MIXED_PROB
```


## 2. Stop the Processes and Collect Coverage Data

After 10 minutes, each of them will automatically generate a statistics file and terminate.

Each output directory will now contain a statistics file (named by strategy: `field_FIXED_PROB_10`, `field_FIXED_PROB_25`, `field_FIXED_PROB_50`, `field_FIXED_PROB_75`, `field_FIXED_PROB_100`, `field_FIXED_SELECTIVE_25_75`, `field_FIXED_SELECTIVE_75_25`, `field_FIXED_RANDOM_PROB`, `field_FIXED_MIXED_PROB`).

## 3. Copy Statistic Files to a Unified Input Directory

Create a new directory called `eval_peripheral_ots/input_dir`, and copy the nine statistics files into it:

```bash
mkdir -p eval_peripheral_ots/input_dir

cp peripheral_ots_FIXED_PROB_10/field_FIXED_PROB_10         eval_peripheral_ots/input_dir
cp peripheral_ots_FIXED_PROB_25/field_FIXED_PROB_25         eval_peripheral_ots/input_dir
cp peripheral_ots_FIXED_PROB_50/field_FIXED_PROB_50         eval_peripheral_ots/input_dir
cp peripheral_ots_FIXED_PROB_75/field_FIXED_PROB_75         eval_peripheral_ots/input_dir
cp peripheral_ots_FIXED_PROB_100/field_FIXED_PROB_100       eval_peripheral_ots/input_dir
cp peripheral_ots_SELECTIVE_25_75/field_SELECTIVE_25_75     eval_peripheral_ots/input_dir
cp peripheral_ots_SELECTIVE_75_25/field_SELECTIVE_75_25     eval_peripheral_ots/input_dir
cp peripheral_ots_RANDOM_PROB/field_RANDOM_PROB             eval_peripheral_ots/input_dir
cp peripheral_ots_MIXED_PROB/field_MIXED_PROB               eval_peripheral_ots/input_dir

````

After running these commands, you should have:

```
eval_peripheral_ots/
└── input_dir/
    ├── field_FIXED_PROB_10
    ├── field_FIXED_PROB_25
    ├── field_FIXED_PROB_50
    ├── field_FIXED_PROB_75
    ├── field_FIXED_PROB_100
    ├── field_SELECTIVE_25_75
    ├── field_SELECTIVE_75_25
    ├── field_RANDOM_PROB 
    └── field_MIXED_PROB
```

## 4. Generate the Coverage PDF
You can generate the coverage plot in RQ4 using: `./gen_rq4.sh <execution duration> <path_to_evaluation_directory>`

This script processes the CSV statistics and generates a PDF plot comparing coverage over time in `<path_to_evaluation_directory>/cov/coverage.pdf`

With all nine strategy outputs in `eval_peripheral_ots/input_dir`, run:

```bash
./gen_rq4.sh 10 $PWD/eval_peripheral_ots
```

This script will process the nine statistic files and produce a PDF showing coverage curves, saving it to:

```
eval_peripheral_ots/cov/coverage.pdf
```

That PDF corresponds to Figure 14 in the paper.




