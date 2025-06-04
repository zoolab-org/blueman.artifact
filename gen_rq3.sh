#!/bin/bash

if [ $# -ne 2 ]; then
	  echo "Usage: $0 <execution duration> <path_to_evaluation_directory>"
	    exit 1
fi

DURATION=$1
EVAL_DIR=$2

DURATIONS=(1 10 60 120 360 720 1440 43200)
if [[ ! " ${DURATIONS[@]} " =~ " ${DURATION} " ]]; then
  echo "Error: invalid execution duration '${DURATION}'"
  echo "Allowed execution durations (in minutes):"
  for d in "${DURATIONS[@]}"; do
    echo "  - $d"
  done
  exit 1
fi

MAX_TIME=$(($DURATION * 60))
DOCKER_IMAGE=blueman_demo
docker run -i --rm -v "$EVAL_DIR":/root/plot -t $DOCKER_IMAGE /bin/bash -lc \
	  "python3 /root/blueman-main/coverage_logger/gen_csv.py /root/plot $MAX_TIME && Rscript /root/blueman-main/coverage_logger/plot.r /root/plot"

