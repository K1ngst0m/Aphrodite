FROM ubuntu:24.04

# Set noninteractive to avoid prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install basic dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    curl \
    git \
    ninja-build \
    wget \
    software-properties-common \
    ccache \
    pkg-config \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libxext-dev \
    libgl1-mesa-dev \
    libssl-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install CMake 3.24.0
RUN wget https://github.com/Kitware/CMake/releases/download/v3.24.0/cmake-3.24.0-linux-x86_64.sh \
    -q -O /tmp/cmake-install.sh \
    && chmod u+x /tmp/cmake-install.sh \
    && /tmp/cmake-install.sh --skip-license --prefix=/usr/local \
    && rm /tmp/cmake-install.sh

# Install Clang 16 - using the correct repository for Ubuntu 24.04
RUN apt-get update && apt-get install -y lsb-release wget software-properties-common gnupg \
    && wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - \
    && echo "deb http://apt.llvm.org/noble/ llvm-toolchain-noble main" > /etc/apt/sources.list.d/llvm.list \
    && apt-get update \
    && apt-get install -y clang-16 lld-16 libc++-16-dev libc++abi-16-dev \
    && update-alternatives --install /usr/bin/clang clang /usr/bin/clang-16 160 \
    && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-16 160 \
    && update-alternatives --install /usr/bin/lld lld /usr/bin/lld-16 160 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install Vulkan dependencies
RUN apt-get update && apt-get install -y \
    vulkan-tools \
    libvulkan1 \
    libvulkan-dev \
    spirv-tools \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Download Vulkan SDK (we'll source setup-env.sh in the entrypoint)
RUN mkdir -p /opt/vulkan && \
    cd /opt/vulkan && \
    wget -q https://sdk.lunarg.com/sdk/download/1.3.243.0/linux/vulkansdk-linux-x86_64-1.3.243.0.tar.gz && \
    tar xzf vulkansdk-linux-x86_64-1.3.243.0.tar.gz && \
    rm vulkansdk-linux-x86_64-1.3.243.0.tar.gz

# Configure ccache
ENV CCACHE_DIR=/ccache
ENV PATH=/usr/lib/ccache:$PATH

# Set working directory
WORKDIR /aphrodite

# Configure cmake settings for proper volume mounting
ENV CMAKE_EXPORT_COMPILE_COMMANDS=ON

# Create entrypoint script
RUN echo '#!/bin/bash\n\
set -e\n\
\n\
# Source Vulkan SDK environment\n\
source /opt/vulkan/1.3.243.0/setup-env.sh\n\
\n\
# Default to clang-release if no preset specified\n\
PRESET=${1:-clang-release}\n\
BUILD_DIR="build-container"\n\
\n\
# Make sure we are using the clang compilers\n\
export CC=clang\n\
export CXX=clang++\n\
\n\
# Clean build directory if requested or if needed to avoid CMake path mismatch issues\n\
if [ "${CLEAN_BUILD:-0}" = "1" ] || [ ! -d "$BUILD_DIR" ]; then\n\
    echo "Cleaning build directory..."\n\
    rm -rf $BUILD_DIR\n\
    mkdir -p $BUILD_DIR\n\
elif [ -f "$BUILD_DIR/CMakeCache.txt" ]; then\n\
    # Check if there is a path mismatch in the CMakeCache.txt\n\
    if grep -q "/home/" "$BUILD_DIR/CMakeCache.txt" && ! grep -q "/aphrodite" "$BUILD_DIR/CMakeCache.txt"; then\n\
        echo "CMake cache contains path mismatch, cleaning build directory..."\n\
        rm -rf $BUILD_DIR\n\
        mkdir -p $BUILD_DIR\n\
    fi\n\
fi\n\
\n\
# Create build directory if it doesn\'t exist\n\
mkdir -p $BUILD_DIR\n\
\n\
# Configure using the specified preset but with custom build directory\n\
cmake -S . -B $BUILD_DIR \\\n\
    -DCMAKE_BUILD_TYPE=$(echo $PRESET | grep -q "debug" && echo "Debug" || echo "Release") \\\n\
    -DCMAKE_C_COMPILER_LAUNCHER=ccache \\\n\
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \\\n\
    $(case "$PRESET" in \\\n\
        *gcc*) echo "-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/toolchain-gcc.cmake" ;; \\\n\
        *clang*) echo "-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/toolchain-clang.cmake" ;; \\\n\
    esac) \\\n\
    $(echo $PRESET | grep -q "ci-" && echo "-DAPH_ENABLE_TESTING=ON" || echo "-DAPH_ENABLE_TESTING=OFF")\n\
\n\
# Build the project\n\
cmake --build $BUILD_DIR -j$(nproc)\n\
\n\
# Run tests if this is a debug build with testing enabled\n\
if [[ "$PRESET" == *debug* && "$PRESET" == ci-* ]]; then\n\
  cd $BUILD_DIR\n\
  ctest -C Debug --output-on-failure\n\
fi\n\
\n\
echo "Build completed successfully! Results are in ./$BUILD_DIR"\n\
' > /usr/local/bin/build.sh \
    && chmod +x /usr/local/bin/build.sh

# Set the entrypoint to the build script
ENTRYPOINT ["/usr/local/bin/build.sh"]

# Default command will build using clang-release preset
CMD ["clang-release"]
