#! /bin/bash
set -e

ACTIONS=(
  gatt_write_peripheral
  hr_peripheral
  sm_pairing_peripheral
  gatt_write_central
  hr_central
  sm_pairing_central
)
MUTATORS=(field afl random)

if [[ $# -ne 3 ]]; then
  echo "Usage: $0 <action> <mutator> <output_dir>"
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
  exit 1
fi

ACTION="$1"
MUTATOR="$2"
OUTPUT_DIR="$3"

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
    target="$ZEPHYR_BUILD_DIR/instrumented_central_gatt_write/zephyr/zephyr.exe"
    attack="$ZEPHYR_BUILD_DIR/peripheral_gatt_write/zephyr/zephyr.exe"
    ;;

  sm_pairing_peripheral)
    target="$BTSTACK_BUILD_DIR/instrumented_sm_pairing_peripheral/zephyr/zephyr.exe"
    attack="$BTSTACK_BUILD_DIR/sm_pairing_central/zephyr/zephyr.exe"
    ;;
  gatt_write_central)
    target="$ZEPHYR_BUILD_DIR/instrumented_central_gatt_write/zephyr/zephyr.exe"
    attack="$ZEPHYR_BUILD_DIR/peripheral_gatt_write/zephyr/zephyr.exe"
    ;;

  hr_central)
    target="$ZEPHYR_BUILD_DIR/instrumented_central_gatt_write/zephyr/zephyr.exe"
    attack="$ZEPHYR_BUILD_DIR/peripheral_gatt_write/zephyr/zephyr.exe"
    ;;

  sm_pairing_central)
    target="$BTSTACK_BUILD_DIR/instrumented_sm_pairing_central/zephyr/zephyr.exe"
    attack="$BTSTACK_BUILD_DIR/sm_pairing_peripheral/zephyr/zephyr.exe"
    ;;

  *)
    echo "Unknown action: $ACTION"
    exit 1
    ;;
esac

# Second parameter: select mutator
case "$MUTATOR" in
  field)
    echo "Applying field-aware mutator"
    ;;

  afl)
    echo "Applying AFL-only mutator"
    ;;

  random)
    echo "Applying random mutator"
    ;;

  *)
    echo "Default mutator: field-aware"
    ;;
esac

echo "Running action '$ACTION' with target=$target and attack=$attack using mutator '$MUTATOR'"
# Execute inside Docker
# Run in Docker with output_dir mounted to run folder
echo "Running Docker container '$DOCKER_IMAGE' with output directory '$OUTPUT_DIR'"
docker run --rm -it \
  -v "$OUTPUT_DIR":/root/blueman-main/run \
  $DOCKER_IMAGE \
  /bin/bash -lc "cd /root/blueman-main && make && cd run && rm -rf output && ./main $BSIM_DIR '$attack' '$target' test"
  #/bin/bash -lc "cd /root/blueman-main && make $MUTATOR && cd run && ./main $BSIM_DIR '$attack' '$target' test"


