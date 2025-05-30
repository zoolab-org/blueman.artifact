#! /bin/bash
set -e

ACTIONS=(
  gatt_write_peripheral
  hr_peripheral
  sm_pairing_peripheral
  le_credit_server
  gatt_write_central
  hr_central
  sm_pairing_central
  le_credit_client
)
MUTATORS=(field afl random)

PKTSELS=(FIXED_PROB_10 FIXED_PROB_25 FIXED_PROB_50 FIXED_PROB_75 FIXED_PROB_100
         SELECTIVE_25_75 SELECTIVE_75_25 RANDOM_PROB MIXED_PROB)

if [[ $# -lt 3 || $# -gt 4 ]]; then
  echo "Usage: $0 <action> <mutator> [packet selection strategies] <output_dir>"
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
  exit 1
fi


if [[ $# -eq 3 ]]; then
  ACTION="$1"
  MUTATOR="$2"
  OUTPUT_DIR="$3"
elif [[ $# -eq 4 ]]; then
  ACTION="$1"
  MUTATOR="$2"
  PKTSEL="${3:-FIXED_PROB_50}"  # Default to FIXED_PROB_50 if not specified
  OUTPUT_DIR="$4"
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

  le_credit_server)
    target="$BTSTACK_BUILD_DIR/instrumented_le_credit_based_flow_control_mode_server/zephyr/zephyr.exe"
    attack="$BTSTACK_BUILD_DIR/le_credit_based_flow_control_mode_client/zephyr/zephyr.exe"
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
  le_credit_client)
    target="$BTSTACK_BUILD_DIR/instrumented_le_credit_based_flow_control_mode_client/zephyr/zephyr.exe"
    attack="$BTSTACK_BUILD_DIR/le_credit_based_flow_control_mode_server/zephyr/zephyr.exe"
    ;;


  *)
    echo "Unknown action: $ACTION"
    exit 1
    ;;
esac

# Third parameter: pktsel parameter for field mutator
if [[ "$MUTATOR" == "field" && $# -eq 4 ]]; then
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
    if [[ $# -eq 4 ]]; then
      echo "Warning: Packet selection strategy '$PKTSEL' is ignored for AFL mutator"
    fi
    ;;

  random)
    echo "Applying random mutator"
    MUTATOR_FLAG="MUTATOR=random"
    if [[ $# -eq 4 ]]; then
      echo "Warning: Packet selection strategy '$PKTSEL' is ignored for random mutator"
    fi
    ;;

  *)
    echo "Default mutator: field-aware"
    MUTATOR_FLAG="PKTSEL_MODE=$PKTSEL"
    ;;
esac

echo "Running action '$ACTION' with target=$target and attack=$attack using mutator '$MUTATOR' and packet selection strategy '$PKTSEL'"
# Execute inside Docker
# Run in Docker with output_dir mounted to run folder
echo "Running Docker container '$DOCKER_IMAGE' with output directory '$OUTPUT_DIR'"
docker run --rm -it \
  -v "$OUTPUT_DIR":/root/blueman-main/run \
  $DOCKER_IMAGE \
  /bin/bash -lc "cd /root/blueman-main && make $MUTATOR_FLAG && cd run && rm -rf output && ./main $BSIM_DIR '$attack' '$target' test"
  # /bin/bash -lc "cd /root/blueman-main && make && cd run && rm -rf output && ./main $BSIM_DIR '$attack' '$target' test"


