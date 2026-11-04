// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

// TBB 2021.5 exposes explicit worker shutdown as a preview feature.
#define TBB_PREVIEW_WAITING_FOR_WORKERS 1
#include <tbb/global_control.h>
#include <tbb/version.h>

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
#if TBB_INTERFACE_VERSION < 12060
  auto scheduler = tbb::task_scheduler_handle::get();
#else
  tbb::task_scheduler_handle scheduler{tbb::attach{}};
#endif
  const auto result = RUN_ALL_TESTS();
  // Wait for worker cleanup before Valgrind checks allocations at process exit.
  tbb::finalize(scheduler);
  return result;
}
