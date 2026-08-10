FROM debian:bookworm-slim AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        libboost-all-dev \
        libssl-dev \
        ninja-build \
        nlohmann-json3-dev \
        pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY CMakeLists.txt ./
COPY include ./include
COPY src ./src

RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build

FROM debian:bookworm-slim AS run

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        libboost-log1.74.0 \
        libboost-thread1.74.0 \
        libboost-system1.74.0 \
        libssl3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app/buzzweb-server
COPY --from=build /app/build/buzzweb_server .

EXPOSE 9291

ENTRYPOINT ["./buzzweb_server"]

