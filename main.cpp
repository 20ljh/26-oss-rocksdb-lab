#include <iostream>
#include <string>
#include <memory>
#include "rocksdb/db.h"
#include "rocksdb/options.h"
using namespace std;
using namespace rocksdb;

int main(){
    // 최신 API 규격: unique_ptr를 사용한 자동 메모리 관리
    unique_ptr<DB> db;
    Options options;
    options.create_if_missing = true;

    string db_path = "./rocksdb_data";
    
    // DB 열기 (Status 객체로 결과 확인)
    Status s = DB::Open(options, db_path, &db);
    
    if(!s.ok()){
        cerr << "RocksDB를 열 수 없습니다: " << s.ToString() << endl;
        return 1;
    }

    // 1. 데이터 저장 (Put)
    string key = "oss:project";
    string value = "26-oss-rocksdb-lab";
    s = db->Put(WriteOptions(), key, value);
    if(s.ok()){
        cout << "저장 완료: [" << key << " -> " << value << "]" << endl;
    }

    // 2. 데이터 조회 (Get)
    string get_value;
    s = db->Get(ReadOptions(), key, &get_value);
    if(s.ok()){
        cout << "조회 결과: " << get_value << endl;
    }

    // 3. 데이터 삭제 (Delete)
    s = db->Delete(WriteOptions(), key);
    if(s.ok()){
        cout << "데이터 삭제됨" << endl;
    }

    // 4. 삭제 확인
    s = db->Get(ReadOptions(), key, &get_value);
    if(s.IsNotFound()){
        cout << "검증 성공: 삭제 후 데이터가 존재하지 않습니다." << endl;
    }

    return 0;
}