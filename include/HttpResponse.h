#include "HttpRequest.h"
#include "ThreadPool.h"
#include <string>
#pragma once
class HttpResponse {
private:
  string status;
  string version;
  string content_type;
  string response;
  string message;
  string path;
  string resource;

public:
  HttpResponse(string resource) {
    this->path = resource;
    this->version = "HTTP/1.1 ";
    this->response = "";
    this->resource = "../content";
    this->resource += resource;
    setContentType();
  }
  bool ends_with(const string resource, const string cmp) {
    if (cmp.size() > resource.size())
      return false;
    else {
      return resource.compare(resource.size() - cmp.size(), cmp.size(), cmp) ==
             0;
    }
  }
  void setContentType() {
    if (ends_with(resource, ".html")) {
      content_type = "Content-Type: text/html\r\n";

    } else if (ends_with(resource, ".css")) {
      content_type = "Content-Type: text/css\r\n";
    } else if (ends_with(resource, ".js")) {
      content_type = "Content-Type: application/javascript\r\n";
    } else if (ends_with(resource, ".png")) {
      content_type = "Content-Type: image/png\r\n";
    } else if (this->resource == "../content/") {
      this->resource += "helloworld.html";
      this->content_type = "Content-Type: text/html\r\n";
    }
  }
  string genrate_response(ClientInfo info, string connectionStatus,
                          thread::id id, int socket) {
    response = "";

    this->getFile(this->resource);
    this->make_response(info, connectionStatus, id, socket);
    return response;
  }
  void set_message(string message) { this->message = message; }
  void set_status(int status) {
    if (status == 200) {
      this->status = to_string(status);
      this->status += " OK\r\n";
    } else {
      this->status = to_string(status);
      this->status += " Not Found\r\n";
    }
  }
  void make_response(ClientInfo info, string connectionStatus, thread::id id,
                     int socket) {
    response += this->version;
    response += this->status;

    response += this->content_type;
    response += "Content-Length: ";

    response += to_string(message.size());
    response += "\r\n";
    response += "Connection: ";
    response += connectionStatus;
    response += "\r\n";
    response += "\r\n";
    response += message;
    response += "\r\n";
    if (stoi(this->status) == 200)
      Logger::log("GET", this->path, stoi(this->status), info.ip, id, socket);
    else
      Logger::error("GET", this->path, stoi(this->status), info.ip);
  }
  string getFile(const string &path) {
    ifstream infile{path};
    if (!infile.is_open()) {
      set_status(404);
      this->content_type = "Content-Type: text/html\r\n";
      infile.open("../content/404.html");

    } else {
      set_status(200);
    }
    string file_contents{istreambuf_iterator<char>(infile),
                         istreambuf_iterator<char>()};
    this->set_message(file_contents);
    return this->message;
  }
};
