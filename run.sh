#!/bin/sh
# TARGET=db_bench -> db_bench 실행 (DB_BENCH_ARGS로 옵션 전달 가능)
# TARGET 미지정 -> 기본 CRUD 실습 실행
#
# 사용 예:
# docker-compose up --build
# TARGET=db_bench docker-compose up
# TARGET=db_bench DB_BENCH_ARGS="--benchmarks=fillseq,stats --statistics" docker-compose up

case "$TARGET" in
    db_bench)
        TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
        LOGFILE="/root/db_bench_log/db_bench_${TIMESTAMP}.log"
        echo "./db_bench ${DB_BENCH_ARGS:-}" > "$LOGFILE"
        ./db_bench ${DB_BENCH_ARGS:-} | tee -a "$LOGFILE"
        ;;
    *)
        ./rocksdb_test
        ;;
esac