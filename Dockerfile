# Stage 1: Build Stage (RocksDB 공식 권장 빌드 방식)
FROM ubuntu:22.04 AS builder

# 의존성 설치 (INSTALL.md 명시 항목들)
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc-11 \
    g++-11 \
    cmake \
    git \
    libgflags-dev \
    libsnappy-dev \
    zlib1g-dev \
    libbz2-dev \
    liblz4-dev \
    libzstd-dev \
    && rm -rf /var/lib/apt/lists/*

# GCC 11을 기본 컴파일러로 설정 (C++20 필수)
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 100 --slave /usr/bin/g++ g++ /usr/bin/g++-11

WORKDIR /src
RUN git clone --depth 1 https://github.com/facebook/rocksdb.git

# [중요] INSTALL.md 권장: Release 모드 정적 라이브러리 빌드
# DEBUG_LEVEL=0: 디버그 정보를 빼고 성능 최적화
# PORTABLE=1: 일반적인 CPU에서 모두 작동하도록 호환성 확보
# -fPIC: 나중에 실행 파일에 묶기 위한 설정
WORKDIR /src/rocksdb
RUN DEBUG_LEVEL=0 PORTABLE=1 EXTRA_CXXFLAGS="-fPIC" make -j$(nproc) static_lib
RUN DEBUG_LEVEL=0 PORTABLE=1 EXTRA_CXXFLAGS="-fPIC" make -j$(nproc) db_bench

# 빌드된 파일들을 시스템 경로로 이동
RUN cp -r include/rocksdb /usr/local/include/ && \
    cp librocksdb.a /usr/local/lib/

# Stage 2: Application Build Stage (우리 앱 빌드)
WORKDIR /app
COPY . .
RUN mkdir build && cd build && \
    cmake .. && \
    make

# Stage 3: Runtime Stage (최종 실행 이미지)
FROM ubuntu:22.04

# 실행 시 필요한 라이브러리 설치
# libgflags2.2: db_bench 런타임 의존성
RUN apt-get update && apt-get install -y \
    libsnappy1v5 liblz4-1 libzstd1 libbz2-1.0 zlib1g \
    libgflags2.2 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /root/
COPY --from=builder /app/build/rocksdb_test .
COPY --from=builder /src/rocksdb/db_bench .

COPY --from=builder /app/run.sh .
RUN sed -i 's/\r//' run.sh && chmod +x run.sh

# TARGET=db_bench -> db_bench 실행 (DB_BENCH_ARGS로 옵션 전달)
# TARGET 미지정 -> rocksdb_test 실행
CMD ["./run.sh"]