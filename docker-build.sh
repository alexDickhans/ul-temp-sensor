#!/bin/bash

# Docker-based nRF Connect SDK Development Environment
# This script provides easy commands for building and flashing firmware

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if Docker is running
check_docker() {
    if ! docker info > /dev/null 2>&1; then
        print_error "Docker is not running. Please start Docker Desktop."
        exit 1
    fi
}

# Function to build the Docker image
build_image() {
    print_status "Building Docker image..."
    docker-compose build
    print_success "Docker image built successfully!"
}

# Function to start the development container
start_container() {
    print_status "Starting development container..."
    docker-compose up -d
    print_success "Container started! You can now run commands inside it."
}

# Function to stop the development container
stop_container() {
    print_status "Stopping development container..."
    docker-compose down
    print_success "Container stopped!"
}

# Function to run a command in the container
run_command() {
    if [ $# -eq 0 ]; then
        print_error "No command specified"
        echo "Usage: $0 run <command>"
        exit 1
    fi
    
    print_status "Running command in container: $*"
    docker-compose exec nrf-dev "$@"
}

# Function to build firmware
build_firmware() {
    print_status "Building firmware in container..."
    docker-compose exec nrf-dev build-firmware
    print_success "Firmware build completed!"
}

# Function to build and flash firmware
build_and_flash() {
    print_status "Building and flashing firmware in container..."
    docker-compose exec nrf-dev build-firmware flash
    print_success "Firmware built and flashed!"
}

# Function to open a shell in the container
open_shell() {
    print_status "Opening shell in container..."
    docker-compose exec nrf-dev /bin/bash
}

# Function to show logs
show_logs() {
    docker-compose logs nrf-dev
}

# Function to show help
show_help() {
    echo "Docker-based nRF Connect SDK Development Environment"
    echo ""
    echo "Usage: $0 <command> [options]"
    echo ""
    echo "Commands:"
    echo "  build-image          Build the Docker image"
    echo "  start                Start the development container"
    echo "  stop                 Stop the development container"
    echo "  restart              Restart the development container"
    echo "  shell                Open a shell in the container"
    echo "  build                Build the firmware"
    echo "  flash                Build and flash the firmware"
    echo "  run <command>        Run a command in the container"
    echo "  logs                 Show container logs"
    echo "  help                 Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 build-image       # Build the Docker image"
    echo "  $0 start             # Start the container"
    echo "  $0 shell             # Open a shell in the container"
    echo "  $0 build             # Build the firmware"
    echo "  $0 flash             # Build and flash the firmware"
    echo "  $0 run west --version # Check west version"
}

# Main script logic
case "${1:-help}" in
    "build-image")
        check_docker
        build_image
        ;;
    "start")
        check_docker
        start_container
        ;;
    "stop")
        check_docker
        stop_container
        ;;
    "restart")
        check_docker
        stop_container
        start_container
        ;;
    "shell")
        check_docker
        open_shell
        ;;
    "build")
        check_docker
        build_firmware
        ;;
    "flash")
        check_docker
        build_and_flash
        ;;
    "run")
        check_docker
        shift
        run_command "$@"
        ;;
    "logs")
        check_docker
        show_logs
        ;;
    "help"|"-h"|"--help")
        show_help
        ;;
    *)
        print_error "Unknown command: $1"
        echo ""
        show_help
        exit 1
        ;;
esac
