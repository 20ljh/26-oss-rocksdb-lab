#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace std;
namespace fs = filesystem;

// Windows의 \r\n 줄바꿈에서 \r 제거
string stripCR(string s){
    if(!s.empty() && s.back() == '\r')
        s.pop_back();
    return s;
}

// 현재 시각을 YYYYMMDD_HHMMSS 형식으로 반환
string currentTimestamp(){
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);
    tm tm_info{};
#ifdef _WIN32
    localtime_s(&tm_info, &t);
#else
    localtime_r(&t, &tm_info);
#endif
    char buf[32];
    strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm_info);
    return buf;
}

int main(int argc, char* argv[]){
    fs::path stats_path = (argc > 1) ? argv[1] : "log_parser_config.txt";
    fs::path logs_dir = "db_bench_log";

    // 설정 파일이 없으면 기본 템플릿 생성 후 종료
    if(!fs::exists(stats_path)){
        ofstream init(stats_path);
        init << "[logs]\n\n[tickers]\n\n[histograms]\n";
        init.close();
        cout << "Config file created: " << stats_path << "\n";
        cout << "Fill in the config and run again.\n";
        return 0;
    }

    ifstream stats_file(stats_path);
    if(!stats_file){
        cerr << "Error: cannot open " << stats_path << "\n";
        return 1;
    }

    vector<string> log_files, tickers, histograms;

    // 섹션 헤더에 따라 각 벡터에 항목 분류
    // 0: none, 1: logs, 2: tickers, 3: histograms
    int section = 0;
    string line;
    while(getline(stats_file, line)){
        line = stripCR(line);
        if(line == "[logs]")       { section = 1; continue; }
        if(line == "[tickers]")    { section = 2; continue; }
        if(line == "[histograms]") { section = 3; continue; }

        if(line.empty())
            continue;

        if(section == 1) { log_files.push_back(line); }
        if(section == 2) { tickers.push_back(line); }
        if(section == 3) { histograms.push_back(line); }
    }
    stats_file.close();

    // * 와일드카드 : db_bench_log/ 모든 로그 파일 수집
    bool has_wildcard = false;
    for(auto& f : log_files)
        if(f == "*") { has_wildcard = true; break; }

    if(has_wildcard){
        log_files.clear();
        if(fs::exists(logs_dir) && fs::is_directory(logs_dir)){
            for(auto& entry : fs::directory_iterator(logs_dir)){
                if(!entry.is_regular_file()) continue;
                string name = entry.path().filename().string();
                if(name.rfind("log_parser_", 0) == 0) continue;
                log_files.push_back(name);
            }
            sort(log_files.begin(), log_files.end());
        }
        if(log_files.empty()){
            cerr << "Error: no log files found in " << logs_dir << "\n";
            return 1;
        }
    }

    if(log_files.empty()){
        cerr << "Error: no log files listed under [logs]\n";
        return 1;
    }

    // 출력 파일: db_bench_log/log_parser_(시각).txt
    fs::path out_path = logs_dir / ("log_parser_" + currentTimestamp() + ".txt");
    ofstream out(out_path);
    if(!out){
        cerr << "Error: cannot create " << out_path << "\n";
        return 1;
    }

    for(int fi = 0; fi < (int)log_files.size(); fi++){
        fs::path log_path = logs_dir / log_files[fi];
        ifstream log_file(log_path);
        if(!log_file){
            cerr << "Warning: cannot open " << log_path << ", skipping\n";
            continue;
        }

        // 첫 줄: db_bench 실행 명령어
        string command;
        getline(log_file, command);
        command = stripCR(command);

        vector<string> matched_tickers(tickers.size());
        vector<string> matched_histograms(histograms.size());

        // 로그를 한 줄씩 읽으며 설정된 항목과 이름이 일치하는 줄 저장
        while(getline(log_file, line)){
            line = stripCR(line);
            for(int i = 0; i < (int)tickers.size(); i++){
                int len = tickers[i].size();
                if((int)line.size() > len &&
                   line.compare(0, len, tickers[i]) == 0 &&
                   line[len] == ' ')      // 이름 뒤에 공백이 있어야 정확히 일치
                    matched_tickers[i] = line;
            }
            for(int i = 0; i < (int)histograms.size(); i++){
                int len = histograms[i].size();
                if((int)line.size() > len &&
                   line.compare(0, len, histograms[i]) == 0 &&
                   line[len] == ' '){
                    // db_bench 결과 형식: name<spaces>:<spaces><stats>
                    size_t pos = len;
                    while(pos < line.size() && line[pos] == ' ') pos++;
                    if(pos < line.size() && line[pos] == ':')
                        matched_histograms[i] = line;
                }
            }
        }
        log_file.close();

        // 파일별 결과 출력
        out << "[" << log_files[fi] << "]\n";
        out << command << "\n";
        for(int i = 0; i < (int)tickers.size(); i++){
            if(!matched_tickers[i].empty())
                out << matched_tickers[i] << "\n";
            else
                out << tickers[i] << " COUNT : NOT FOUND\n";
        }
        for(int i = 0; i < (int)histograms.size(); i++){
            if(!matched_histograms[i].empty())
                out << matched_histograms[i] << "\n";
            else
                out << histograms[i] << " : NOT FOUND\n";
        }
        out << "\n";
    }

    out.close();
    cout << "Saved: " << out_path << "\n";
    return 0;
}
