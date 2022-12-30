#include <GLFW/glfw3.h>
#include "dispatch.h"

namespace ImPlay {
Dispatch::Dispatch() {}

Dispatch::~Dispatch() {}

void Dispatch::run(Fn func, void *data) {
  auto item = new Dispatch::Item{func, data, false, false};
  push(item);

  {
    std::unique_lock<std::mutex> lk(mutex);
    cv.wait(lk, [&]() { return item->completed; });
    delete item;
  }
}

void Dispatch::push(Fn func, void *data) {
  auto item = new Dispatch::Item{func, data, false, true};
  push(item);
}

void Dispatch::push(Item *item) {
  std::unique_lock<std::mutex> lk(mutex);
  queue.push(item);
  glfwPostEmptyEvent();
}

void Dispatch::process() {
  std::unique_lock<std::mutex> lk(mutex);
  while (!queue.empty()) {
    auto item = queue.front();
    queue.pop();

    item->func(item->data);

    if (item->asynchronous)
      delete item;
    else
      item->completed = true;

    lk.unlock();
    cv.notify_one();
    lk.lock();
  }
}
}  // namespace ImPlay