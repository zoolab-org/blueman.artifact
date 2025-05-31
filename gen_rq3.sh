#!/bin/bash

if [ $# -ne 1 ]; then
	  echo "Usage: $0 <path_to_evaluation_directory>"
	    exit 1
fi

EVAL_DIR=$1
DOCKER_IMAGE=blueman_demo
docker run -it --rm -v "$EVAL_DIR":/root/plot -t $DOCKER_IMAGE /bin/bash -lc \
	  "python3 /root/blueman-main/coverage_logger/gen_csv.py /root/plot && Rscript /root/blueman-main/coverage_logger/plot.r /root/plot"

