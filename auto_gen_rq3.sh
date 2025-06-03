#!/bin/bash

set -e

if [ $# -ne 2 ]; then
  echo "Usage: $0 <action> <duration_in_minutes>"
  exit 1
fi

ACTION="$1"
DURATION="$2"
WRAPPER_DIR="${ACTION}_rq3"
EVAL_DIR="${WRAPPER_DIR}/eval_${ACTION}"
MUTATORS=("field" "afl" "random")

# Create directories for the wrapper and evaluation
mkdir -p "$WRAPPER_DIR"

# 1. execute each mutator strategy
for MUTATOR in "${MUTATORS[@]}"; do
  OUTDIR="${WRAPPER_DIR}/${ACTION}_${MUTATOR}"
  mkdir -p "$OUTDIR"
  echo "==> Running: $ACTION $MUTATOR"
  (
    cd "$PWD"
    ./run.sh "$ACTION" "$MUTATOR" "$DURATION" "$PWD/$OUTDIR" > /dev/null
  )&
  
done

# wait
sleep $(($DURATION * 60 + 30))

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
./gen_rq3.sh "$PWD/${EVAL_DIR}"

echo "==> Coverage plot generation completed for $ACTION, the plot is saved in ${EVAL_DIR}/cov/"
