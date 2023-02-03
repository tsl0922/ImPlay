// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include "helpers.h"
#include "views/view.h"

namespace ImPlay::Views {
View::View(Config *config, Dispatch *dispatch, Mpv *mpv) {
  this->config = config;
  this->dispatch = dispatch;
  this->mpv = mpv;
}

void View::dispatch_sync(Dispatch::Fn fn, void *data) { dispatch->sync(fn, data); }

void View::dispatch_async(Dispatch::Fn fn, void *data) { dispatch->async(fn, data); }
}  // namespace ImPlay::Views