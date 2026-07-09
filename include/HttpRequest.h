#include "Logger.h"
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#pragma once

using namespace std;
class HttpRequest {
private:
  string method;
  string path;
  string version;

  map<string, string> headers;

public:
  string get_connection_status() {
    auto it = headers.find("Connection");

    if (it == headers.end())
      return "keep-alive";
    if (it->second.empty())
      return "keep-alive";

    return it->second;
  }
  void parse(const string &buffer) {

    string request = buffer;
    istringstream stream(request);
    string word;
    getline(stream, word);
    istringstream firstLine(word);
    firstLine >> this->method >> this->path >> this->version;
    while (getline(stream, word)) {
      if (word.empty() || word == "\r") {
        break;
      }

      size_t pos = word.find(':');
      string key = word.substr(0, pos);
      string value = word.substr(pos + 1);
      if (value[0] == ' ' && !value.empty()) {
        value.erase(0, 1);
      }
      this->headers[key] = value;
    }
    this->Print_request();
  }
  void Print_request() {
    cout << "Method : " << this->method << endl;
    cout << "Path : " << this->path << endl;
    cout << "Version : " << this->version << endl;
    for (const auto &headers : headers) {
      cout << headers.first << ": " << headers.second << endl;
    }

    cout << endl;
  }
  string get_path() { return this->path; }
};