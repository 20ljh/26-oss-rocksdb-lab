# 🧪 RocksDB CRUD 실습 및 벤치마킹

이 프로젝트는 **RocksDB 소스 코드를 직접 빌드**하고, 최신 **C++20 환경**에서 데이터베이스의 기본적인 CRUD 기능을 실습할 수 있도록 구성되었습니다.

또한 RocksDB 공식 벤치마크 도구인 **`db_bench`** 를 통해 MemTable 크기, Bloom filter, Compaction 방법 등 다양한 옵션을 조절하며 성능 차이를 실험적으로 분석할 수 있습니다. 실행 결과는 로그 파일로 자동 저장됩니다.

## 📋 사전 준비 사항 (Prerequisites)

이 프로젝트를 실행하기 위해서는 사용자의 컴퓨터에 다음 도구가 설치되어 있어야 합니다.

- [**Git**](https://git-scm.com/) : 코드 다운로드용
- [**Docker Desktop**](https://www.docker.com/products/docker-desktop/) : 서버 및 빌드 환경 실행용 (실행 상태여야 함)

## 🛠️ 설치 가이드 (Installation)

### 1. 프로젝트 다운로드 (Clone)

터미널(Git Bash, CMD, PowerShell 등)을 열고 아래 명령어를 입력하여 프로젝트를 다운로드합니다.

```
# 코드 다운로드
git clone https://github.com/20ljh/26-oss-rocksdb-lab.git

# 프로젝트 폴더로 이동
cd 26-oss-rocksdb-lab
```

### 2. 프로젝트 빌드 및 실행

Docker를 이용해 RocksDB를 컴파일하고 테스트 프로그램을 실행합니다. **정적 링크(Static Linking)** 방식으로 빌드되어 별도의 라이브러리 설정이 필요 없습니다.

빌드가 완료되면 Docker 컨테이너가 자동으로 `main.cpp`의 테스트 코드를 실행합니다.

```
# 빌드 및 실행
docker-compose up --build
```

**참고** : 처음 실행 시 RocksDB 전체 소스 코드를 컴파일하므로, 컴퓨터 사양에 따라 **5~10분 정도 소요**될 수 있습니다. (한 번 빌드된 이후에는 캐시를 사용하여 매우 빠르게 실행됩니다.)

## 🖥️ 실행 가이드 (Usage)

### 1. CRUD 실습
터미널 로그를 통해 다음 단계별 작동을 확인할 수 있습니다.
```
# CRUD 실습
docker-compose up
```

- 1단계 : **데이터 저장 (Create)** : K-V쌍 (A, 123), (B, 456), (C, 789)가 저장됩니다.

- 2단계 : **데이터 조회 (Read)** : A, B, C 세 키의 값이 순서대로 출력됩니다.

- 3단계 : **데이터 수정 (Update)** : B의 값을 456789로 수정합니다. (RocksDB는 별도의 Update API 없이 `Put`으로 동일 키를 덮어씁니다.)

- 4단계 : **데이터 삭제 및 검증 (Delete)** : A를 삭제합니다. (RocksDB는 해당 키에 툼스톤(Tombstone) 마크를 남겨 논리적으로 삭제합니다.)

- 5단계 : **데이터 영속성 확인 (Persistence)** : 프로젝트 폴더 내 **`rocksdb_data/`** 폴더에 `LOG`, `MANIFEST`, `CURRENT` 파일이 생성되어 있는지 확인합니다. (Docker 바인드 마운트를 통해 컨테이너 내부 데이터가 호스트에 실시간으로 저장됩니다.)

### 2. 성능 벤치마킹 (db_bench)

`TARGET=db_bench`로 실행하면 RocksDB 공식 벤치마크 도구인 `db_bench`를 사용할 수 있습니다.

```
# 기본 실행 (fillseq + stats)
TARGET=db_bench docker-compose up

# 옵션 지정 실행
TARGET=db_bench DB_BENCH_ARGS="--benchmarks=fillseq,readrandom --bloom_bits=10 --statistics" docker-compose up
```

실행 결과는 **`db_bench_log/`** 폴더에 `db_bench_(실행시각).log` 파일로 자동 저장됩니다. 로그 파일 첫 줄에 실행 명령어, 이후 줄에 전체 출력 내역이 기록됩니다.

### 3. 로그 파서 (log_parser)

`db_bench` 로그에서 원하는 Ticker, Histogram 항목만 추출하는 도구입니다. 호스트에서 직접 빌드하여 사용할 수 있습니다.

**빌드**
```
g++ -std=c++20 -o log_parser.exe log_parser.cpp
```

**실행**
```
./log_parser.exe
```

최초 실행 시 `log_parser_config.txt`가 자동 생성되고 프로그램이 종료됩니다. 아래 예시를 참고하여 파일을 채운 뒤 다시 실행하세요.

`[logs]`에 `*`를 단독으로 입력하면 `db_bench_log/` 내 모든 로그 파일을 자동으로 파싱 대상으로 지정합니다.

```
[logs]
*

[tickers]
rocksdb.bloom.filter.useful

[histograms]
rocksdb.db.get.micros

[read/write]
fillseq
```

- **`[logs]`** : 파싱할 `db_bench_log/` 내 로그 파일명 (여러 줄 가능). `*` 입력 시 폴더 내 전체 로그 파일 자동 포함
- **`[tickers]`** : 추출할 Ticker 항목명 (`rocksdb.bloom.filter.useful COUNT : ...` 형식)
- **`[histograms]`** : 추출할 Histogram 항목명 (`rocksdb.db.get.micros P50 : ...` 형식)
- **`[read/write]`** : 추출할 벤치마크 결과 항목명 (`fillseq      :       ... micros/op` 형식)

출력 결과는 **`db_bench_log/log_parser_(실행시각).txt`** 로 저장됩니다.

## 🛑 컨테이너 종료 및 정리

테스트가 끝나면 다음 명령어로 컨테이너를 종료합니다.

```
# 컨테이너 종료
docker-compose down
```

## 📂 프로젝트 구조

- **`Dockerfile`** : Ubuntu 22.04 기반의 멀티 스테이지 설계도. RocksDB 소스를 빌드하고 실행 파일만 추출하여 최적화된 이미지를 만듭니다.
- **`docker-compose.yml`** : 컨테이너 환경 설정 및 로컬 폴더와의 데이터 동기화(Volume)를 정의합니다.
- **`run.sh`** : 컨테이너 진입점 스크립트. `TARGET` 환경 변수에 따라 CRUD 실습(`rocksdb_test`) 또는 벤치마크(`db_bench`)를 실행합니다.
- **`main.cpp`** : C++20 스마트 포인터를 활용한 RocksDB CRUD 실습 코드입니다.
- **`CMakeLists.txt`** : C++ 표준(20) 설정 및 RocksDB 정적 라이브러리 연결을 위한 빌드 스크립트입니다.
- **`rocksdb_data/`** : RocksDB의 실제 데이터베이스 파일들이 저장되는 물리적 공간입니다.
- **`db_bench_log/`** : `db_bench` 실행 결과가 타임스탬프 단위로 자동 저장되는 로그 폴더입니다.
- **`log_parser.cpp`** : `db_bench` 로그에서 원하는 통계 항목만 추출하는 파서입니다. 호스트에서 직접 빌드합니다.
- **`log_parser_config.txt`** : 파서 설정 파일. 최초 실행 시 자동 생성됩니다.

**2026-1 오픈소스SW분석(빅데이터) RocksDB 실습 이주형**