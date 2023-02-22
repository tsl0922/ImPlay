// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace ImPlay {

class Dispatch {
 public:
  using Fn = std::function<void()>;

  // Submits a block object for execution and returns after that block finishes executing
  void sync(Fn func);

  // Submits a block object for execution (on the next event loop) and returns immediately
  void async(Fn func);

  // process all the tasks in the queue, should be called in the main thread only
  void process();

  std::function<void()> &wakeup() { return wakeupFn; }

 private:
  struct Item {
    Fn func;
    bool completed;
    bool asynchronous;
  };

  void push(Item *item);

  std::function<void()> wakeupFn;
  std::queue<Item *> queue;
  std::mutex mutex;
  std::condition_variable cond;
};
}  // namespace ImPlay