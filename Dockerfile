# nRF Connect SDK Development Environment
FROM ubuntu:22.04

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV ZEPHYR_BASE=/workspace/ncs/zephyr
ENV ZEPHYR_SDK_INSTALL_DIR=/workspace/ncs/toolchains/v2.4.0/opt/zephyr-sdk
ENV PATH=$PATH:/workspace/ncs/toolchains/v2.4.0/opt/zephyr-sdk/sysroots/x86_64-pokysdk-linux/usr/bin

# Install system dependencies
RUN apt-get update && apt-get install -y \
    git \
    wget \
    curl \
    cmake \
    ninja-build \
    python3 \
    python3-pip \
    python3-setuptools \
    python3-wheel \
    build-essential \
    pkg-config \
    libssl-dev \
    libusb-1.0-0-dev \
    udev \
    && rm -rf /var/lib/apt/lists/*

# Install west
RUN pip3 install west

# Create workspace directory
WORKDIR /workspace

# Install Zephyr SDK using west
RUN west init -m https://github.com/nrfconnect/sdk-nrf --mr v2.4.0 /tmp/ncs \
    && cd /tmp/ncs \
    && west update \
    && west zephyr-export \
    && cp -r /tmp/ncs /workspace/ncs

# Download and install nRF Command Line Tools
RUN wget https://www.nordicsemi.com/-/media/Software-and-other-downloads/Desktop-software/nRF-command-line-tools/sw/Versions-10-x-x/10-18-1/nRF-Command-Line-Tools_10_18_1_Linux-amd64.tar.gz \
    && tar -xzf nRF-Command-Line-Tools_10_18_1_Linux-amd64.tar.gz \
    && cd nRF-Command-Line-Tools_10_18_1_Linux-amd64 \
    && ./install.sh \
    && cd .. \
    && rm -rf nRF-Command-Line-Tools_10_18_1_Linux-amd64*

# Install J-Link software
RUN wget https://www.segger.com/downloads/jlink/JLink_Linux_V788a_x86_64.tgz \
    && tar -xzf JLink_Linux_V788a_x86_64.tgz -C /opt \
    && rm JLink_Linux_V788a_x86_64.tgz

# Add J-Link to PATH
ENV PATH=$PATH:/opt/JLink_Linux_V788a_x86_64

# Create udev rules for J-Link
RUN echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="1366", ATTR{idProduct}=="0101", MODE="0666"' > /etc/udev/rules.d/99-jlink.rules

# Set up Zephyr environment from the pre-installed NCS
RUN cd /workspace/ncs && west zephyr-export

# Create a script to build and flash
RUN echo '#!/bin/bash\n\
echo "Building firmware..."\n\
cd /workspace/mcu_firmware\n\
west build -b nrf52840dk_nrf52840\n\
echo "Build complete!"\n\
if [ "$1" = "flash" ]; then\n\
    echo "Flashing firmware..."\n\
    west flash\n\
    echo "Flash complete!"\n\
fi' > /usr/local/bin/build-firmware && chmod +x /usr/local/bin/build-firmware

# Set up west configuration
RUN west config --global --unset ncs.root || true
RUN west config --global ncs.root /workspace/ncs

# Set default command
CMD ["/bin/bash"]

