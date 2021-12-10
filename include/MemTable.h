//
// Created by pospelov on 08.12.2021.
//

#ifndef LOCAL_STORAGE_SRC_MEMTABLE_H_
#define LOCAL_STORAGE_SRC_MEMTABLE_H_

#include <map>
#include <utility>

#include "Config.h"
#include "Entry.h"
#include "IRunnable.h"
#include "Log.h"

class MemTable : public IRunnable {
public:
  explicit MemTable(const Config &config)
      : config(config), log(config.log_file_name) {}

  bool put(const Entry &entry) {
    log.putToLog(entry.key, entry.data);
    memory[entry.key] = entry.data;
    return true;
  }

  bool get(Key key, std::string &data) {
    if (memory.find(key) == memory.end()) {
      return false;
    }
    data = memory[key];
    return true;
  }

  void start() override {
    log.start();

    auto values = log.readAll();
    std::cerr << values.size() << " entries\n";
    memory = std::map(values.rbegin(), values.rend());

    is_running = true;
  }

  void stop() override { is_running = false; }

  bool isRunning() const override { return is_running; }

  ~MemTable() override = default;

private:
  const Config config;

  bool is_running = false;

  Log log;
  std::map<Key, std::string> memory;
};

#endif // LOCAL_STORAGE_SRC_MEMTABLE_H_
