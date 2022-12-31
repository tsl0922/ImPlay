#pragma once
#include <functional>

namespace ImPlay {
using dispatch_fn = std::function<void(void *)>;

// Submits a block object for execution and returns after that block finishes executing
void dispatch_sync(dispatch_fn func, void *data);

// Submits a block object for execution (on the next event loop) and returns immediately
void dispatch_async(dispatch_fn func, void *data);

// process all the tasks in the queue, should be called in the main thread only
void dispatch_process();

// wake up the processing thread (main thread)
void dispatch_wakeup();
}  // namespace ImPlay