#include <iostream>
#include <string>
#include <memory>
#include "rocksdb/db.h"
#include "rocksdb/options.h"
using namespace std;
using namespace rocksdb;

static void check(const Status& s, const string& label){
    if(!s.ok()){
        cerr << "[오류] " << label << ": " << s.ToString() << endl;
    }
}

int main(){
    unique_ptr<DB> db;
    Options options;
    options.create_if_missing = true;

    string db_path = "./rocksdb_data";

    Status s = DB::Open(options, db_path, &db);
    if(!s.ok()){
        cerr << "RocksDB를 열 수 없습니다: " << s.ToString() << endl;
        return 1;
    }

    // 1. Create (Put)
    cout << "\n[1] Create" << endl;
    s = db->Put(WriteOptions(), "A", "123"); check(s, "Put A");
    if(s.ok()){
        cout << "저장: A -> 123" << endl;
    }

    s = db->Put(WriteOptions(), "B", "456"); check(s, "Put B");
    if(s.ok()){
        cout << "저장: B -> 456" << endl;
    }

    s = db->Put(WriteOptions(), "C", "789"); check(s, "Put C");
    if(s.ok()){
        cout << "저장: C -> 789" << endl;
    }

    // 2. Read (Get)
    cout << "\n[2] Read" << endl;
    string val;
    for(const string& k : {"A", "B", "C"}){
        s = db->Get(ReadOptions(), k, &val); check(s, "Get " + k);
        if(s.ok()){
            cout << "조회: " << k << " -> " << val << endl;
        }
    }

    // 3. Update (Put으로 덮어쓰기)
    cout << "\n[3] Update" << endl;
    s = db->Put(WriteOptions(), "B", "456789"); check(s, "Update B");
    if(s.ok()){
        db->Get(ReadOptions(), "B", &val);
        cout << "갱신: B -> " << val << endl;
    }

    // 4. Delete
    cout << "\n[4] Delete" << endl;
    s = db->Delete(WriteOptions(), "A"); check(s, "Delete A");
    if(s.ok()){
        cout << "삭제: A" << endl;
    }

    s = db->Get(ReadOptions(), "A", &val);
    if(s.IsNotFound()){
        cout << "확인: A는 존재하지 않습니다." << endl;
    }

    return 0;
}
