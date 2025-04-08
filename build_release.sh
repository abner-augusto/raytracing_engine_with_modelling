#!/bin/bash

# Stop on errors
set -e

BUILD_DIR="build"

echo "📁 Creating build directory: $BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "⚙️ Configuring project with CMake (Release mode)..."
cmake .. -DCMAKE_BUILD_TYPE=Release

echo "🔨 Building project..."
make -j$(nproc)

echo "✅ Build completed!"

BINARY="./RaytracerCG1"
if [ -f "$BINARY" ]; then
    echo "🚀 Ready to run: $BUILD_DIR/RaytracerCG1"
else
    echo "❌ Build succeeded, but binary '$BINARY' not found. Check your CMake target name."
fi