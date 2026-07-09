
#pragma once
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <thread>

using namespace std;
class Logger {
private:
  inline static mutex mtx;

public:
  static void log(const string &method, const string &path, int status,
                  string ip, std::thread::id id, int socket) {
    lock_guard<mutex> lock(mtx);
    ofstream my_file("logs.txt", std::ios::app);
    if (!my_file)
      return;
    auto now = chrono::system_clock::now();

    time_t current = chrono::system_clock::to_time_t(now);
    tm tm_buf;
    localtime_r(&current, &tm_buf);
    my_file << put_time(&tm_buf, "[%Y-%m-%d %H:%M:%S] ") << ip << " " << method
            << " " << path << " " << status << " thread_id:" << id
            << " socket id:" << socket << endl;
  }
  static void info(const string &Client, const string &ip) {
    lock_guard<mutex> lock(mtx);
    ofstream my_file("logs.txt", std::ios::app);
    if (!my_file)
      return;
    auto now = chrono::system_clock::now();
    time_t current = chrono::system_clock::to_time_t(now);
    tm tm_buf;
    localtime_r(&current, &tm_buf);
    my_file << put_time(&tm_buf, "[%Y-%m-%d %H:%M:%S] ") << ip << " " << "INFO "
            << Client << "\n";
  }
  static void error(const string &method, const string &path, int status,
                    string ip) {
    lock_guard<mutex> lock(mtx);
    ofstream my_file("error_logs.txt", std::ios::app);
    if (!my_file)
      return;
    auto now = chrono::system_clock::now();

    time_t current = chrono::system_clock::to_time_t(now);
    tm tm_buf;
    localtime_r(&current, &tm_buf);
    my_file << put_time(&tm_buf, "[%Y-%m-%d %H:%M:%S] ") << ip << " " << method
            << " " << path << " " << status << '\n';
  }
};