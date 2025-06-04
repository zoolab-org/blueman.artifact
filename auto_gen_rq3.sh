#!/bin/bash

set -e

if [ $# -ne 2 ]; then
  echo "Usage: $0 <action> <execution duration>"
  exit 1
fi

ACTION="$1"
DURATION="$2"
WRAPPER_DIR="${ACTION}_${DURATION}_rq3"
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
  echo "Available actions:"
  for a in "${ACTIONS[@]}"; do
    echo "  - $a"
  done
  exit 1
fi

MUTATORS=("field" "afl" "random")

DURATIONS=(1 10 60 120 360 720 1440 43200)
if [[ ! " ${DURATIONS[@]} " =~ " ${DURATION} " ]]; then
  echo "Error: invalid execution duration '${DURATION}'"
  echo "Available execution durations (in minutes):"
  for d in "${DURATIONS[@]}"; do
    echo "  - $d"
  done
  exit 1
fi

# Create directories for the wrapper and evaluation
mkdir -p "$WRAPPER_DIR"

# 1. execute each mutator strategy
for MUTATOR in "${MUTATORS[@]}"; do
  OUTDIR="${WRAPPER_DIR}/${ACTION}_${MUTATOR}"
  mkdir -p "$OUTDIR"
  echo "==> Running: $ACTION $MUTATOR"
  ./run.sh "$ACTION" "$MUTATOR" "$DURATION" "$PWD/$OUTDIR" > /dev/null &
  
done

# wait
sleep $(($DURATION * 60 + 50))

# 2. create a unified eval directory
mkdir -p "${EVAL_DIR}/input_dir"

# 3. collect outputs from each strategy
for MUTATOR in "${MUTATORS[@]}"; do
  SRC="${WRAPPER_DIR}/${ACTION}_${MUTATOR}/${MUTATOR}"
  if [ -f "$SRC" ]; then
    cp "$SRC" "${EVAL_DIR}/input_dir/"
  else
    echo "Warning: $SRC does not exist"
  fi
done

# 4. generate plots
echo "==> Generating coverage plot for $ACTION"
./gen_rq3.sh "$DURATION" "$PWD/${EVAL_DIR}" 

echo "==> Coverage plot generation completed for $ACTION, duration time: $DURATION minutes. The plot is saved in ${EVAL_DIR}/cov/"
