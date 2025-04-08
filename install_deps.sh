#!/bin/bash

# Stop on errors
set -e

echo "🔧 Updating package list..."
sudo apt update

echo "📦 Installing required packages..."
sudo apt install -y \
    build-essential \
    cmake \
    g++ \
    libsdl2-dev \
    libsdl2-ttf-dev \
    libomp-dev \
    git

echo "✅ Dependencies installed!"