// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include "config.h"
#include "dispatch.h"
#include "mpv.h"

namespace ImPlay::Views {
class View {
 public:
  View(Config *config, Dispatch *dispatch, Mpv *mpv);
  View() = default;
  virtual ~View() = default;

  virtual void draw() = 0;
  virtual void show() { m_open = true; }

  inline void dispatch_sync(Dispatch::Fn fn) { dispatch->sync(fn); };
  inline void dispatch_async(Dispatch::Fn fn) { dispatch->async(fn); };

 protected:
  Config *config = nullptr;
  Dispatch *dispatch = nullptr;
  Mpv *mpv = nullptr;
  bool m_open = false;
};
}  // namespace ImPlay::Views