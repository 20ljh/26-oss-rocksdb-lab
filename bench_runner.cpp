#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;
namespace fs = filesystem;

const string CONFIG_FILE = "bench_option.txt";

string stripCR(string s) {
    if (!s.empty() && s.back() == '\r') s.pop_back();
    return s;
}

string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t");
    return s.substr(a, b - a + 1);
}

// [manipulated] 값 분리용 - 쉼표가 구분자
vector<string> splitComma(const string& s) {
    vector<string> result;
    istringstream ss(s);
    string tok;
    while (getline(ss, tok, ','))
        result.push_back(trim(tok));
    return result;
}

struct ManipParam {
    string key;
    vector<string> values;
};

struct BenchConfig {
    vector<ManipParam> manipulated;
    vector<pair<string, string>> controlled; // {key, value} - value="" 이면 boolean 플래그
};

BenchConfig parseConfig(const string& filename) {
    BenchConfig cfg;
    ifstream file(filename);
    if (!file) {
        cerr << "Error: cannot open " << filename << "\n";
        exit(1);
    }

    int section = 0; // 0=none, 1=manipulated, 2=controlled
    string line;
    while (getline(file, line)) {
        line = trim(stripCR(line));
        if (line.empty()) continue;
        if (line == "[manipulated]") { section = 1; continue; }
        if (line == "[controlled]")  { section = 2; continue; }

        size_t eq = line.find('=');

        if (section == 1) {
            if (eq == string::npos) continue; // manipulated는 '=' 필수
            ManipParam p;
            p.key    = trim(line.substr(0, eq));
            p.values = splitComma(trim(line.substr(eq + 1)));
            if (!p.values.empty()) cfg.manipulated.push_back(p);
        } else if (section == 2) {
            if (eq == string::npos)
                cfg.controlled.push_back({line, ""});                          // boolean 플래그
            else
                cfg.controlled.push_back({trim(line.substr(0, eq)),
                                          trim(line.substr(eq + 1))});         // key=value
        }
    }
    return cfg;
}

// i번째 실행에서 각 [manipulated] 파라미터의 값 인덱스 계산
// 첫 번째 파라미터가 가장 빠르게 변함 (column-major)
vector<int> getIndices(const vector<ManipParam>& params, int runIdx) {
    vector<int> idx(params.size());
    for (size_t j = 0; j < params.size(); j++) {
        idx[j]  = runIdx % (int)params[j].values.size();
        runIdx /= (int)params[j].values.size();
    }
    return idx;
}

// DB_BENCH_ARGS 문자열 조합 - controlled 먼저, manipulated 뒤
string buildArgs(const BenchConfig& cfg, const vector<int>& idx) {
    string args;
    for (const auto& [key, val] : cfg.controlled) {
        if (!args.empty()) args += " ";
        args += val.empty() ? ("--" + key) : ("--" + key + "=" + val);
    }
    for (size_t i = 0; i < cfg.manipulated.size(); i++) {
        if (!args.empty()) args += " ";
        args += "--" + cfg.manipulated[i].key + "=" + cfg.manipulated[i].values[idx[i]];
    }
    return args;
}

int main() {
    // 설정 파일 없으면 기본 템플릿 생성 후 종료
    if (!fs::exists(CONFIG_FILE)) {
        ofstream init(CONFIG_FILE);
        init << "[manipulated]\n\n[controlled]\n";
        cout << "Config file created: " << CONFIG_FILE << "\n";
        cout << "Fill in and run again.\n";
        return 0;
    }

    BenchConfig cfg = parseConfig(CONFIG_FILE);

    if (cfg.manipulated.empty()) {
        cerr << "Error: no parameters in [manipulated]\n";
        return 1;
    }

    int total = 1;
    for (const auto& p : cfg.manipulated) total *= (int)p.values.size();

    // 실행 계획 출력
    cout << "=== bench_runner: " << total << " runs planned ===\n";
    for (int i = 0; i < total; i++) {
        auto idx = getIndices(cfg.manipulated, i);
        cout << "  [" << (i + 1) << "/" << total << "] ";
        for (size_t j = 0; j < cfg.manipulated.size(); j++) {
            if (j) cout << ", ";
            cout << cfg.manipulated[j].key << "=" << cfg.manipulated[j].values[idx[j]];
        }
        cout << "\n";
    }
    cout << "=========================================\n\n";

    for (int i = 0; i < total; i++) {
        auto idx  = getIndices(cfg.manipulated, i);
        string args = buildArgs(cfg, idx);

        cout << "\n>>> Run [" << (i + 1) << "/" << total << "]\n";
        for (size_t j = 0; j < cfg.manipulated.size(); j++) {
            if (j) cout << ", ";
            cout << cfg.manipulated[j].key << "=" << cfg.manipulated[j].values[idx[j]];
        }
        cout << "\nDB_BENCH_ARGS=\"" << args << "\"\n";
        cout << "-----------------------------------------\n";

        // 환경변수 직접 설정 후 docker-compose 호출 (sh 의존 없이 cmd.exe에서 동작)
        _putenv_s("TARGET", "db_bench");
        _putenv_s("DB_BENCH_ARGS", args.c_str());
        system("docker-compose --progress plain up --abort-on-container-exit");
        // 종료된 컨테이너 및 네트워크까지 제거해 다음 실행과 격리
        int ret = system("docker-compose --progress plain down");

        if (ret != 0)
            cerr << "Warning: docker-compose exited with code " << ret << "\n";

        cout << "--- Run [" << (i + 1) << "/" << total << "] done ---\n";
    }

    cout << "\n=== All " << total << " runs completed! ===\n";
    return 0;
}
