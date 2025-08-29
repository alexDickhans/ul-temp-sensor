# Docker Development Environment Setup

This document explains how to use the Docker-based development environment for building and flashing the nRF52840 firmware.

## Prerequisites

1. **Docker Desktop** installed on your macOS system
2. **USB access** for J-Link programming (the container needs access to USB devices)

## Quick Start

### 1. Build the Docker Image

```bash
./docker-build.sh build-image
```

This will create a Docker image with:
- Ubuntu 22.04 base
- nRF Connect SDK v2.4.0
- Zephyr SDK v0.13.1
- nRF Command Line Tools
- J-Link software
- All necessary build tools

### 2. Start the Development Container

```bash
./docker-build.sh start
```

### 3. Build the Firmware

```bash
./docker-build.sh build
```

### 4. Build and Flash the Firmware

```bash
./docker-build.sh flash
```

## Detailed Usage

### Available Commands

```bash
./docker-build.sh <command>
```

| Command | Description |
|---------|-------------|
| `build-image` | Build the Docker image |
| `start` | Start the development container |
| `stop` | Stop the development container |
| `restart` | Restart the development container |
| `shell` | Open a shell in the container |
| `build` | Build the firmware |
| `flash` | Build and flash the firmware |
| `run <command>` | Run a specific command in the container |
| `logs` | Show container logs |
| `help` | Show help message |

### Interactive Development

To open a shell in the container for interactive development:

```bash
./docker-build.sh shell
```

Inside the container, you can:
- Use `west` commands directly
- Navigate to `/workspace/mcu_firmware`
- Build with `west build -b nrf52840dk_nrf52840`
- Flash with `west flash`
- Use `west --help` for more options

### Running Custom Commands

```bash
# Check west version
./docker-build.sh run west --version

# List available boards
./docker-build.sh run west boards

# Clean build
./docker-build.sh run west build -b nrf52840dk_nrf52840 -p always

# Build with specific configuration
./docker-build.sh run west build -b nrf52840dk_nrf52840 -d build-debug -- -DCONFIG_LOG_DEFAULT_LEVEL=4
```

## USB Device Access

The Docker setup is configured to access USB devices for J-Link programming. The container runs with:

- `privileged: true` - Full access to host devices
- USB device mounting (`/dev/bus/usb`)
- udev rules for J-Link access

### Troubleshooting USB Access

If you encounter USB access issues:

1. **Check if J-Link is detected:**
   ```bash
   ./docker-build.sh run lsusb
   ```

2. **Check J-Link software:**
   ```bash
   ./docker-build.sh run JLinkExe --version
   ```

3. **Test J-Link connection:**
   ```bash
   ./docker-build.sh run JLinkExe -device nRF52840_XXAA -if SWD -speed 4000
   ```

4. **If USB access fails:**
   - Ensure Docker Desktop has permission to access USB devices
   - Try restarting Docker Desktop
   - Check if your J-Link device is properly connected

## File System

Your project directory is mounted at `/workspace` inside the container:

- **Host**: `/Users/alex/Personal/Fun/temp-sensor-code`
- **Container**: `/workspace`

Changes made in the container are immediately reflected on your host system.

## Build Artifacts

Build outputs are stored in:
- **Host**: `mcu_firmware/build/`
- **Container**: `/workspace/mcu_firmware/build/`

## Environment Variables

The container has these environment variables set:
- `ZEPHYR_BASE=/workspace/zephyr`
- `ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk`
- `PATH` includes Zephyr SDK and J-Link tools

## Advantages of Docker Setup

1. **Isolated Environment**: No conflicts with system packages
2. **Reproducible**: Same environment across different machines
3. **Complete Toolchain**: All tools pre-installed and configured
4. **Easy Cleanup**: Remove container to start fresh
5. **Cross-Platform**: Works on any system with Docker

## Troubleshooting

### Build Issues

1. **Clean build:**
   ```bash
   ./docker-build.sh run west build -b nrf52840dk_nrf52840 -p always
   ```

2. **Check build configuration:**
   ```bash
   ./docker-build.sh run west build -b nrf52840dk_nrf52840 -- -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
   ```

3. **View build logs:**
   ```bash
   ./docker-build.sh logs
   ```

### Container Issues

1. **Rebuild image:**
   ```bash
   ./docker-build.sh stop
   docker-compose build --no-cache
   ./docker-build.sh start
   ```

2. **Reset container:**
   ```bash
   ./docker-build.sh stop
   docker-compose down -v
   ./docker-build.sh start
   ```

### Performance Tips

1. **Exclude build directories** from Docker context (already done in `.dockerignore`)
2. **Use volume mounts** for large directories
3. **Build in stages** for faster iteration

## Alternative: Direct Docker Commands

If you prefer using Docker commands directly:

```bash
# Build image
docker-compose build

# Start container
docker-compose up -d

# Run command
docker-compose exec nrf-dev west build -b nrf52840dk_nrf52840

# Stop container
docker-compose down
```

## Next Steps

After setting up the Docker environment:

1. **Build the firmware**: `./docker-build.sh build`
2. **Flash to device**: `./docker-build.sh flash`
3. **Test the sensor node**: Monitor BLE advertisements
4. **Deploy to Raspberry Pi**: Follow the deployment guide

The Docker environment provides a complete, isolated development setup that eliminates the complexity of installing and configuring the nRF Connect SDK toolchain on your local system.
