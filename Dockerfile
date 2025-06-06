FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
SHELL ["/bin/bash", "-lc"]

RUN dpkg --add-architecture i386 

RUN apt-get update && apt-get upgrade -y && \
    apt-get install --no-install-recommends -y git cmake ninja-build gperf \
    ccache dfu-util device-tree-compiler wget \
    python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file \
    make gcc g++-multilib libsdl2-dev libmagic1 python3-venv r-base libjson-c-dev

RUN apt-get -y install gcc-multilib g++-multilib

COPY patch/ /patch
COPY blueman-main /root/blueman-main
COPY build_scripts /root/build_scripts


RUN cd /root && \
    git clone https://github.com/google/AFL.git AFL_FOR_ZEPHYR_BLE && \
    cd AFL_FOR_ZEPHYR_BLE && \
    git apply /patch/zephyr_afl.patch && \
    make && \
    mkdir -p /root/bin && \
    cp afl-gcc /root/bin/gcc

RUN cd /root && \
    wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.5-1/zephyr-sdk-0.16.5-1_linux-x86_64.tar.xz && \
    wget -O - https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.5-1/sha256.sum | shasum --check --ignore-missing


RUN cd /root && \
    tar xvf zephyr-sdk-0.16.5-1_linux-x86_64.tar.xz 

RUN cd /root/zephyr-sdk-0.16.5-1 && \
    echo -e 'y\ny\n' | ./setup.sh

RUN python3 -m venv /root/zephyrproject/.venv && \
    source /root/zephyrproject/.venv/bin/activate && \
    pip install --no-cache-dir west && \
    west init -m https://github.com/zephyrproject-rtos/zephyr --mr main /root/zephyrproject && \
    cd /root/zephyrproject/zephyr && \
    git checkout f211cd6345555086307da8c4998783ebe0ec0027 && \
    cd /root/zephyrproject && \
    west config manifest.group-filter -- +babblesim && \
    west update && \
    west zephyr-export && \
    cd ${ZEPHYR_BASE}/root/zephyrproject/tools/bsim && \
    make everything -j8

RUN source /root/zephyrproject/.venv/bin/activate && \
    pip install --no-cache-dir -r /root/zephyrproject/zephyr/scripts/requirements.txt

RUN cd /root/zephyrproject/modules/bsim_hw_models/nrf_hw_models/ && \
    git apply /patch/bsim.patch

RUN cp /patch/btfuzz.h /root/zephyrproject/modules/bsim_hw_models/nrf_hw_models/src/HW_models/

RUN cd /root/zephyrproject/zephyr && \
    git apply /patch/zephyr.patch && \
    echo -e 'export BSIM_OUT_PATH=/root/zephyrproject/tools/bsim\nexport BSIM_COMPONENTS_PATH=${BSIM_OUT_PATH}/components/' > /root/.zephyrrc

RUN chmod +x /root/build_scripts/build_zephyr_examples.sh && /root/build_scripts/build_zephyr_examples.sh

RUN cd root/ && git clone https://github.com/bluekitchen/btstack.git && \
    cd btstack && git checkout v1.6.2 && \
    git apply /patch/btstack.patch

RUN cd /root && \
    git clone https://github.com/google/AFL.git AFL_FOR_BTSTACK && \
    cd AFL_FOR_BTSTACK && \
    git apply /patch/btstack_afl.patch && \
    make && \
    cp afl-gcc /root/bin/gcc

RUN chmod +x /root/build_scripts/build_btstack_examples.sh && /root/build_scripts/build_btstack_examples.sh

RUN cd /root/blueman-main && \
    mkdir run && cp -r /root/zephyrproject/tools/bsim/lib ./ && \
    make

WORKDIR /root/

CMD ["bash", "-lc", "source /root/zephyrproject/.venv/bin/activate && source /root/.zephyrrc && bash"]
