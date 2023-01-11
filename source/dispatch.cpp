#include "dispatch.h"

namespace ImPlay {
void Dispatch::push(Dispatch::Item *item) {
  std::unique_lock<std::mutex> lk(mutex);
  queue.push(item);
  if (wakeupFn) wakeupFn();
}

void Dispatch::async(Dispatch::Fn func, void *data) {
  auto item = new Dispatch::Item{std::move(func), data, false, true};
  push(item);
}

void Dispatch::sync(Dispatch::Fn func, void *data) {
  auto item = new Dispatch::Item{std::move(func), data, false, false};
  push(item);

  {
    std::unique_lock<std::mutex> lk(mutex);
    cond.wait(lk, [&]() { return item->completed; });
    delete item;
  }
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
    cond.notify_one();
    lk.lock();
  }
}
}  // namespace ImPlay
