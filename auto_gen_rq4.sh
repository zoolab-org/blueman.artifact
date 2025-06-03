#!/bin/bash

set -e

if [ $# -ne 2 ]; then
  echo "Usage: $0 <action> <duration_in_minutes>"
  exit 1
fi

ACTION="$1"
DURATION="$2"
WRAPPER_DIR="${ACTION}_rq4"
EVAL_DIR="${WRAPPER_DIR}/eval_${ACTION}"
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

# Create directories for the wrapper and evaluation
mkdir -p "$WRAPPER_DIR"

# 1. execute nine strategies
for STRATEGY in "${STRATEGIES[@]}"; do
  OUTDIR="${WRAPPER_DIR}/${ACTION}_${STRATEGY}"
  mkdir -p "$OUTDIR"
  echo "==> Running: $ACTION $STRATEGY"
  ./run.sh "$ACTION" field "$STRATEGY" "$DURATION" "$PWD/$OUTDIR" > /dev/null &
done

# wait
sleep $(($DURATION * 60 + 30))

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
./gen_rq4.sh "$PWD/${EVAL_DIR}"

echo "==> Coverage plot generation completed for $ACTION, the plot is saved in ${EVAL_DIR}/cov/"
