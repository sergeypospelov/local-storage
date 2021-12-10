//
// Created by pospelov on 09.12.2021.
//

#ifndef LOCAL_STORAGE_SRC_LOG_H_
#define LOCAL_STORAGE_SRC_LOG_H_

#include <fstream>
#include <utility>
#include <vector>

#include "Entry.h"
#include "IRunnable.h"
#include "byteIO.h"

class Log : public IRunnable {
public:
  explicit Log(const std::string &log_filename) : log_filename(log_filename) {
  }
  std::vector<std::pair<Key, std::string>> readAll() {
    std::ifstream in_log(log_filename, std::ios::binary);

    Entry entry;
    std::vector<std::pair<Key, std::string>> entries;
    while (in_log >> entry) {
      entries.emplace_back(entry.key, entry.data);
    }
    return entries;
  }

  void putToLog(uint64_t key, const std::string &value) {
    out_log << Entry{key, value};
    out_log.flush();
  }

  void start() override {
    out_log.open(log_filename, std::ios::binary | std::ios::app);
    is_running = true;
  }

  void stop() override {
    out_log.close();
    is_running = false;
  }

  bool isRunning() const override { return is_running; }

  ~Log() override = default;

private:
  const std::string log_filename;

  std::ofstream out_log;

  bool is_running = false;
};

#endif // LOCAL_STORAGE_SRC_LOG_H_
