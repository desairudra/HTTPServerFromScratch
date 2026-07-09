#include "../include/ThreadPool.h"
#include "../include/Server.h"

using namespace std;

ThreadPool::ThreadPool(size_t numberofthreads, Server *server)
    : stop(false), server(server) {
  for (int i = 0; i < numberofthreads; i++) {
    workers.emplace_back([this] {
      for (;;) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this] { return stop || !tasks.empty(); });
        if (stop && tasks.empty()) {
          return;
        }
        ClientInfo task = tasks.front();
        tasks.pop();
        lock.unlock();
        this->server->handleclient(task, this_thread::get_id());
      }
    });
  }
}

void ThreadPool::enqueue(ClientInfo sockfd) {
  unique_lock<mutex> lock(mtx);
  tasks.push(sockfd);
  lock.unlock();
  cv.notify_one();
}
ThreadPool::~ThreadPool() {
  {
    unique_lock<mutex> lock(mtx);
    stop = true;
  }

  cv.notify_all();

  for (auto &worker : workers) {
    worker.join();
  }
}
