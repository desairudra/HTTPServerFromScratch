#pragma once
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
using namespace std;
class Server;
struct ClientInfo {
  int socket;
  string ip;
  string recvBuffer;

  string sendBuffer;
};
class ThreadPool {
private:
  Server *server;

public:
  vector<thread> workers;
  queue<ClientInfo> tasks;
  mutex mtx;
  condition_variable cv;
  bool stop;

  ThreadPool(size_t numberofthreads, Server *server);

  void enqueue(ClientInfo sockfd);
  ~ThreadPool();
};