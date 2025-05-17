#!/bin/bash

# Exit if any command fails
set -e

# Build the project
cmake -S . -B build
cmake --build build

# Run the server from the project root so it can access ./public
./build/http_server
