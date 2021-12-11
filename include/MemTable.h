//
// Created by pospelov on 08.12.2021.
//

#ifndef LOCAL_STORAGE_SRC_MEMTABLE_H_
#define LOCAL_STORAGE_SRC_MEMTABLE_H_

#include <map>
#include <utility>

#include <atomic>
#include <mutex>
#include <thread>

#include "Config.h"
#include "Entry.h"
#include "IRunnable.h"
#include "Log.h"
#include "SSTableRegistry.h"

class MemTable : public IRunnable {
public:
  explicit MemTable(const Config &config, SSTableRegistry &registry)
      : config(config), log(config.log_file_name), registry(registry) {}

  ~MemTable() override = default;

  bool put(const Entry &entry) {
    log.putToLog(entry.key, entry.data);
    memory[entry.key] = entry.data;
    mem_table_size += entry.data.size();

    checkMemTableSize();

    return true;
  }

  bool get(Key key, std::string &data) {
    std::lock_guard lock_guard(mem_usage_mutex);
    if (memory.find(key) == memory.end()) {
      if (is_dumping && old_memory.find(key) != old_memory.end()) {
        data = old_memory[key];
        return true;
      }
      return false;
    }
    data = memory[key];
    return true;
  }

  void start() override {
    log.start();

    auto [values, size] = log.readAll();
    memory = std::map(values.rbegin(), values.rend());
    mem_table_size = size;

    checkMemTableSize();

    if (mem_table_dumper.joinable()) {
      mem_table_dumper.join();
    }

    is_running = true;
  }

  void stop() override {
    if (mem_table_dumper.joinable()) {
      mem_table_dumper.join();
    }

    is_running = false;
  }

  bool isRunning() const override { return is_running; }

  uint64_t getMemTableSize() const { return mem_table_size; }

private:
  void dumpMemTable() {
    old_memory = memory;
    {
      std::lock_guard lock_guard(mem_usage_mutex);
      memory.clear();
    }
  }

  bool checkMemTableSize() {
    if (getMemTableSize() > config.mem_table_dump_size && !is_dumping) {
      is_dumping = true;
      mem_table_dumper = std::thread(&MemTable::dumpMemTable, this);
      return true;
    }
    return false;
  }

private:
  const Config &config;

  bool is_running = false;
  bool is_dumping = false;

  uint64_t mem_table_size = 0;

  Log log;
  std::map<Key, std::string> memory;
  std::map<Key, std::string> old_memory;

  std::mutex mem_usage_mutex;
  std::thread mem_table_dumper;

  SSTableRegistry &registry;
};

#endif // LOCAL_STORAGE_SRC_MEMTABLE_H_
