#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace ImPlay {
class Dispatch {
 public:
  Dispatch();
  ~Dispatch();

  using Fn = std::function<void(void *)>;

  void run(Fn func, void *data);
  void push(Fn func, void *data);
  void process();

 private:
  struct Item {
    Fn func;
    void *data;
    bool completed;
    bool asynchronous;
  };

  void push(Item *item);

  std::queue<Item*> queue;
  std::mutex mutex;
  std::condition_variable cv;
};
}  // namespace ImPlay