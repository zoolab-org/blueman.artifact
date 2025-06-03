#! /bin/bash
set -e

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
MUTATORS=(field afl random)

PKTSELS=(FIXED_PROB_10 FIXED_PROB_25 FIXED_PROB_50 FIXED_PROB_75 FIXED_PROB_100
         SELECTIVE_25_75 SELECTIVE_75_25 RANDOM_PROB MIXED_PROB)

EXE_DURS=(1 10 60 120 360 720 1440 43200)

if [[ $# -lt 4 || $# -gt 5 ]]; then
  echo "Usage: $0 <action> <mutator> [packet selection strategies] <execution duration> <output_dir>"
  echo
  echo "Available actions:"
  for a in "${ACTIONS[@]}"; do
    echo "  - $a"
  done
  echo
  echo "Available mutators:"
  for m in "${MUTATORS[@]}"; do
    echo "  - $m"
  done
  echo
  echo "Available packet selection strategies (for field mutator only):"
  for p in "${PKTSELS[@]}"; do
    echo "  - $p"
  done
  echo
  echo "Available execution duration (in minutes):"
  for p in "${EXE_DURS[@]}"; do
    echo "  - $p"
  done
  exit 1
fi


if [[ $# -eq 4 ]]; then
  ACTION="$1"
  MUTATOR="$2"
  SET_TIMER="$3"
  OUTPUT_DIR="$4"
elif [[ $# -eq 5 ]]; then
  ACTION="$1"
  MUTATOR="$2"
  PKTSEL="$3"  
  SET_TIMER="$4"
  OUTPUT_DIR="$5"
fi


target=""
attack=""

# Validate output directory exists
if [[ ! -d "$OUTPUT_DIR" ]]; then
  echo "Error: output directory '$OUTPUT_DIR' does not exist."
  exit 1
fi

ZEPHYR_BUILD_DIR=/root/zephyrproject/zephyr/build
BTSTACK_BUILD_DIR=/root/btstack/port/zephyr/build
BSIM_DIR=/root/zephyrproject/tools/bsim/bin/bs_2G4_phy_v1
DOCKER_IMAGE=blueman_demo

# First parameter: select target and attack based on ACTION
case "$ACTION" in
  gatt_write_peripheral)
    target="$ZEPHYR_BUILD_DIR/instrumented_peripheral_gatt_write/zephyr/zephyr.exe"
    attack="$ZEPHYR_BUILD_DIR/central_gatt_write/zephyr/zephyr.exe"
    ;;

  hr_peripheral)
    target="$ZEPHYR_BUILD_DIR/instrumented_peripheral_hr/zephyr/zephyr.exe"
    attack="$ZEPHYR_BUILD_DIR/central_hr/zephyr/zephyr.exe"
    ;;

  sm_pairing_peripheral)
    target="$BTSTACK_BUILD_DIR/instrumented_sm_pairing_peripheral/zephyr/zephyr.exe"
    attack="$BTSTACK_BUILD_DIR/sm_pairing_central/zephyr/zephyr.exe"
    ;;

  peripheral_ots)
    target="$ZEPHYR_BUILD_DIR/instrumented_peripheral_ots/zephyr/zephyr.exe"
    attack="$ZEPHYR_BUILD_DIR/central_otc/zephyr/zephyr.exe"
    ;;

  le_credit_server)
    target="$BTSTACK_BUILD_DIR/instrumented_le_credit_based_flow_control_mode_server/zephyr/zephyr.exe"
    attack="$BTSTACK_BUILD_DIR/le_credit_based_flow_control_mode_client/zephyr/zephyr.exe"
    ;;

  ots_peripheral)
    target="$ZEPHYR_BUILD_DIR/instrumented_peripheral_ots/zephyr/zephyr.exe"
    attack="$ZEPHYR_BUILD_DIR/central_otc/zephyr/zephyr.exe"
    ;;

  gatt_write_central)
    target="$ZEPHYR_BUILD_DIR/instrumented_central_gatt_write/zephyr/zephyr.exe"
    attack="$ZEPHYR_BUILD_DIR/peripheral_gatt_write/zephyr/zephyr.exe"
    ;;

  hr_central)
    target="$ZEPHYR_BUILD_DIR/instrumented_central_hr/zephyr/zephyr.exe"
    attack="$ZEPHYR_BUILD_DIR/peripheral_hr/zephyr/zephyr.exe"
    ;;

  sm_pairing_central)
    target="$BTSTACK_BUILD_DIR/instrumented_sm_pairing_central/zephyr/zephyr.exe"
    attack="$BTSTACK_BUILD_DIR/sm_pairing_peripheral/zephyr/zephyr.exe"
    ;;

  central_otc)
    target="$ZEPHYR_BUILD_DIR/instrumented_central_otc/zephyr/zephyr.exe"
    attack="$ZEPHYR_BUILD_DIR/peripheral_ots/zephyr/zephyr.exe"
    ;;

  le_credit_client)
    target="$BTSTACK_BUILD_DIR/instrumented_le_credit_based_flow_control_mode_client/zephyr/zephyr.exe"
    attack="$BTSTACK_BUILD_DIR/le_credit_based_flow_control_mode_server/zephyr/zephyr.exe"
    ;;

  otc_central) 
    target="$ZEPHYR_BUILD_DIR/instrumented_central_otc/zephyr/zephyr.exe"
    attack="$ZEPHYR_BUILD_DIR/peripheral_ots/zephyr/zephyr.exe"
    ;;

  *)
    echo "Unknown action: $ACTION"
    exit 1
    ;;
esac

# Third parameter: pktsel parameter for field mutator
if [[ "$MUTATOR" == "field" && $# -eq 5 ]]; then
  if [[ ! " ${PKTSELS[@]} " =~ " ${PKTSEL} " ]]; then
    echo "Unknown packet selection strategy '$PKTSEL'"
    echo "Available strategies: ${PKTSELS[*]}"
    exit 1
  fi
fi

# Second parameter: select mutator
case "$MUTATOR" in
  field)
    echo "Applying field-aware mutator with packet selection strategy '$PKTSEL'"
    MUTATOR_FLAG="PKTSEL_MODE=$PKTSEL"
    ;;

  afl)
    echo "Applying AFL-only mutator"
    MUTATOR_FLAG="MUTATOR=afl"
    if [[ $# -eq 5 ]]; then
      echo "Warning: Packet selection strategy '$PKTSEL' is ignored for AFL mutator"
    fi
    ;;

  random)
    echo "Applying random mutator"
    MUTATOR_FLAG="MUTATOR=random"
    if [[ $# -eq 5 ]]; then
      echo "Warning: Packet selection strategy '$PKTSEL' is ignored for random mutator"
    fi
    ;;

  *)
    echo "Default mutator: field-aware"
    MUTATOR=field
    MUTATOR_FLAG="PKTSEL_MODE=$PKTSEL"
    ;;
esac

case "$SET_TIMER" in
  1)
    SET_TIMER=60
    ;;

  10)
    SET_TIMER=600
    ;;

  60)
    SET_TIMER=3600
    ;;

  120)
    SET_TIMER=7200
    ;;

  360)
    SET_TIMER=21600
    ;;

  720)
    SET_TIMER=43200
    ;;

  1440)
    SET_TIMER=86400
    ;;
	
  *)
    echo "Invalid execution duration"
    exit 1
    ;;
esac

echo "Running action '$ACTION' with target=$target and attack=$attack using mutator '$MUTATOR' and packet selection strategy '$PKTSEL' and execution duration '$SET_TIMER'"
# Execute inside Docker
# Run in Docker with output_dir mounted to run folder
echo "Running Docker container '$DOCKER_IMAGE' with output directory '$OUTPUT_DIR'"

if [[ "$MUTATOR" == "field" && -n "$PKTSEL" ]]; then
  FINAL_ARG="${MUTATOR}_${PKTSEL}"
else
  FINAL_ARG="$MUTATOR"
fi

docker run --rm -i \
  -v "$OUTPUT_DIR":/root/blueman-main/run \
  $DOCKER_IMAGE \
  /bin/bash -lc "cd /root/blueman-main && make build_stat SET_TIMER=$SET_TIMER $MUTATOR_FLAG && cd run && rm -rf output && ./main $BSIM_DIR '$attack' '$target' $FINAL_ARG"


