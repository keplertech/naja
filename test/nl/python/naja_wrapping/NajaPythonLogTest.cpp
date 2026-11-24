// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <Python.h>
#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "NajaLog.h"

extern "C" PyObject* PyInit_naja();

TEST(NajaPythonLogTest, callbacksFromNativeThreadsWithoutGIL) {
  ASSERT_EQ(0, PyImport_AppendInittab("naja", &PyInit_naja));
  PyConfig config;
  PyConfig_InitPythonConfig(&config);
  if (const char* executable = std::getenv("NAJA_TEST_PYTHON_EXECUTABLE")) {
    // Bazel supplies the hermetic interpreter in runfiles. Its location lets
    // CPython discover the matching standard library instead of its build prefix.
    auto path = std::filesystem::absolute(executable).string();
    auto status = PyConfig_SetBytesString(&config, &config.program_name, path.c_str());
    ASSERT_FALSE(PyStatus_Exception(status)) << status.err_msg;
  }
  auto status = Py_InitializeFromConfig(&config);
  PyConfig_Clear(&config);
  ASSERT_FALSE(PyStatus_Exception(status)) << status.err_msg;
  ASSERT_EQ(0, PyRun_SimpleString(R"PY(
import naja, threading
naja.set_console_logging(False)
naja.setLogLevel('trace')
records = []
threads = set()
def receive(level, message):
    records.append((level, message))
    threads.add(threading.get_ident())
handle = naja.add_log_callback(receive, level='warning')
)PY"));
  auto* state = PyEval_SaveThread();
  std::vector<std::thread> workers;
  std::atomic<unsigned> ready{0};
  for (unsigned i = 0; i < 4; ++i) {
    workers.emplace_back([&] {
      ++ready;
      while (ready != 4) std::this_thread::yield();
      for (unsigned j = 0; j < 100; ++j) NAJA_LOG_WARN("worker payload {}", 23);
    });
  }
  PyEval_RestoreThread(state);
  EXPECT_EQ(0, PyRun_SimpleString(R"PY(
for _ in range(100):
    temporary = naja.add_log_callback(lambda *args: None)
    naja.remove_log_callback(temporary)
)PY"));
  state = PyEval_SaveThread();
  for (auto& worker: workers) worker.join();
  PyEval_RestoreThread(state);
  EXPECT_EQ(0, PyRun_SimpleString(R"PY(
assert len(records) == 400, len(records)
assert len(threads) == 4, threads
assert all(record == ('warning', 'worker payload 23') for record in records)
naja.remove_log_callback(handle)
)PY"));
  state = PyEval_SaveThread();
  std::thread([] { NAJA_LOG_WARN("after removal"); }).join();
  PyEval_RestoreThread(state);
  EXPECT_EQ(0, PyRun_SimpleString("assert len(records) == 400"));
  // Leave a registered callback behind to exercise module teardown.
  EXPECT_EQ(0, PyRun_SimpleString("naja.add_log_callback(receive)"));
  EXPECT_EQ(0, Py_FinalizeEx());
  naja::log::clearSinks();
}
