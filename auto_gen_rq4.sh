#!/bin/bash

set -e

if [ $# -ne 2 ]; then
  echo "Usage: $0 <action> <execution duration>"
  exit 1
fi

ACTION="$1"
DURATION="$2"
WRAPPER_DIR="${ACTION}_${DURATION}_rq4"
EVAL_DIR="${WRAPPER_DIR}/eval_${ACTION}"

ACTIONS=(
  gatt_write_peripheral
  hr_peripheral
  sm_pairing_peripheral
  le_credit_server
  ots_peripheral
  gatt_write_central
  hr_central
  sm_pairing_central
  le_credit_client
  otc_central
)
if [[ ! " ${ACTIONS[@]} " =~ " ${ACTION} " ]]; then
  echo "Error: unknown action '${ACTION}'."
  echo "Allowed actions:"
  for a in "${ACTIONS[@]}"; do
    echo "  - $a"
  done
  exit 1
fi

STRATEGIES=(
  "FIXED_PROB_10"
  "FIXED_PROB_25"
  "FIXED_PROB_50"
  "FIXED_PROB_75"
  "FIXED_PROB_100"
  "SELECTIVE_25_75"
  "SELECTIVE_75_25"
  "RANDOM_PROB"
  "MIXED_PROB"
)

DURATIONS=(1 10 60 120 360 720 1440 43200)
if [[ ! " ${DURATIONS[@]} " =~ " ${DURATION} " ]]; then
  echo "Error: invalid execution duration '${DURATION}'"
  echo "Allowed execution durations (in minutes):"
  for d in "${DURATIONS[@]}"; do
    echo "  - $d"
  done
  exit 1
fi

# Create directories for the wrapper and evaluation
mkdir -p "$WRAPPER_DIR"

# 1. execute nine strategies
for STRATEGY in "${STRATEGIES[@]}"; do
  OUTDIR="${WRAPPER_DIR}/${ACTION}_${STRATEGY}"
  mkdir -p "$OUTDIR"
  echo "==> Running: $ACTION $STRATEGY"
  ./run.sh "$ACTION" field "$STRATEGY" "$DURATION" "$PWD/$OUTDIR" > /dev/null &
done
wait


# 2. create a unified eval directory
mkdir -p "${EVAL_DIR}/input_dir"

# 3. collect outputs from each strategy
for STRATEGY in "${STRATEGIES[@]}"; do
  SRC="${WRAPPER_DIR}/${ACTION}_${STRATEGY}/field_${STRATEGY}"
  if [ -f "$SRC" ]; then
    cp "$SRC" "${EVAL_DIR}/input_dir/"
  else
    echo "Warning: $SRC does not exist"
  fi
done

# 4. generate plots
echo "==> Generating coverage plot for $ACTION"
./gen_rq4.sh "$DURATION" "$PWD/${EVAL_DIR}" 

echo "==> Coverage plot generation completed for $ACTION, duration time: $DURATION minutes. The plot is saved in ${EVAL_DIR}/cov/"
