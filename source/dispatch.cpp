#include <queue>
#include <mutex>
#include <condition_variable>
#include <GLFW/glfw3.h>
#include "dispatch.h"

namespace ImPlay {
struct dispatch_item {
  dispatch_fn func;
  void *data;
  bool completed;
  bool asynchronous;
};

std::queue<dispatch_item *> dispatch_queue;
std::mutex dispatch_mutex;
std::condition_variable dispatch_cond;

void dispatch_push(dispatch_item *item) {
  std::unique_lock<std::mutex> lk(dispatch_mutex);
  dispatch_queue.push(item);

  dispatch_wakeup();
}

void dispatch_async(dispatch_fn func, void *data) {
  auto item = new dispatch_item{func, data, false, true};
  dispatch_push(item);
}

void dispatch_sync(dispatch_fn func, void *data) {
  auto item = new dispatch_item{func, data, false, false};
  dispatch_push(item);

  {
    std::unique_lock<std::mutex> lk(dispatch_mutex);
    dispatch_cond.wait(lk, [&]() { return item->completed; });
    delete item;
  }
}

void dispatch_process() {
  std::unique_lock<std::mutex> lk(dispatch_mutex);
  while (!dispatch_queue.empty()) {
    auto item = dispatch_queue.front();
    dispatch_queue.pop();

    item->func(item->data);

    if (item->asynchronous)
      delete item;
    else
      item->completed = true;

    lk.unlock();
    dispatch_cond.notify_one();
    lk.lock();
  }
}

void dispatch_wakeup() { glfwPostEmptyEvent(); }
}  // namespace ImPlay