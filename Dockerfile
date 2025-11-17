# Multi-stage build for Grand Strategy Simulation Engine
FROM gcc:13 AS builder

# Install CMake
RUN apt-get update && apt-get install -y \
    cmake \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source files
COPY include/ ./include/
COPY src/ ./src/
COPY CMakeLists.txt ./

# Build the application
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . && \
    strip KernelSim OpinionSim

# Runtime stage - debian with full gcc runtime
FROM debian:bookworm-slim

# Install basic runtime dependencies
RUN apt-get update && apt-get install -y \
    libgomp1 \
    && rm -rf /var/lib/apt/lists/*

# Create app directory and data directory
WORKDIR /app
RUN mkdir -p /app/data

# Copy built executables from builder
COPY --from=builder /app/build/KernelSim /app/
COPY --from=builder /app/build/OpinionSim /app/

# Copy newer libstdc++ from gcc:13 builder (from /usr/local/lib64)
COPY --from=builder /usr/local/lib64/libstdc++.so.6.0.32 /usr/lib/x86_64-linux-gnu/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libgcc_s.so.1 /usr/lib/x86_64-linux-gnu/

# Recreate symlinks for libstdc++
RUN cd /usr/lib/x86_64-linux-gnu && \
    ln -sf libstdc++.so.6.0.32 libstdc++.so.6 && \
    ln -sf libstdc++.so.6 libstdc++.so

# Create non-root user
RUN useradd -m -u 1000 simuser && \
    chown -R simuser:simuser /app

USER simuser

# Set library path to ensure newer libstdc++ is found
ENV LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu

# Volume for output data
VOLUME ["/app/data"]

# Default command - interactive mode
ENTRYPOINT ["/app/KernelSim"]

# Labels
LABEL org.opencontainers.image.title="Grand Strategy Simulation Engine"
LABEL org.opencontainers.image.description="Agent-based simulation for emergent cultures, movements, and ideologies"
LABEL org.opencontainers.image.version="0.2.0"
LABEL org.opencontainers.image.authors="Grand Strategy Team"
