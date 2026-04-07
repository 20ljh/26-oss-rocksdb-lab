cmake_minimum_required(VERSION 3.10)
project(rocksdb_test)

# RocksDB 최신 버전 요구사항인 C++20 설정
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 시스템에 설치된 정적 라이브러리(librocksdb.a)를 명시적으로 찾음
find_library(ROCKSDB_LIB NAMES librocksdb.a rocksdb)
find_package(Threads REQUIRED)

add_executable(rocksdb_test main.cpp)

# RocksDB와 그 의존성들을 정적으로 연결 (Link)
target_link_libraries(rocksdb_test 
    ${ROCKSDB_LIB}
    Threads::Threads
    z bz2 lz4 zstd snappy
)