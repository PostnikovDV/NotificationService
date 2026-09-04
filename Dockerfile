# ============================================================
# NotificationService Dockerfile (с vcpkg)
# ============================================================

FROM ubuntu:22.04 AS builder

# Установка базовых инструментов (включая git для vcpkg)
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    tar \
    zip \
    unzip \
    autoconf \
    pkg-config \
    libssl-dev \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# ============================================================
# Установка vcpkg
# ============================================================

WORKDIR /opt
RUN git clone https://github.com/microsoft/vcpkg.git && \
    ./vcpkg/bootstrap-vcpkg.sh

ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

# ============================================================
# Установка библиотек через vcpkg
# ============================================================

RUN vcpkg install \
    boost-asio:x64-linux \
    boost-beast:x64-linux \
    boost-system:x64-linux \
    boost-uuid:x64-linux \
    nlohmann-json:x64-linux \
    librdkafka:x64-linux

# ============================================================
# Копирование исходников
# ============================================================

WORKDIR /app
COPY . .

# ============================================================
# Сборка
# ============================================================

RUN mkdir -p build && cd build && \
    cmake .. \
        -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DVCPKG_TARGET_TRIPLET=x64-linux && \
    make -j$(nproc)

# ============================================================
# Runtime
# ============================================================

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libssl3 \
    zlib1g \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /opt/vcpkg/installed/x64-linux/lib/*.so* /usr/local/lib/
RUN ldconfig

WORKDIR /app

COPY --from=builder /app/build/NotificationService /app/NotificationService

EXPOSE 8083

CMD ["/app/NotificationService", "0.0.0.0", "8083", "4"]