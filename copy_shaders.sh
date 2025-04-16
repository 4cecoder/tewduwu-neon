#!/bin/bash

# Make sure assets directory exists
mkdir -p assets/shaders

# Copy all WGSL shaders to assets directory
cp -v src/shaders/*.wgsl assets/shaders/

echo "Shaders copied successfully!" 