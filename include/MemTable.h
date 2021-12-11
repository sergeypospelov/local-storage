//
// Created by pospelov on 08.12.2021.
//

#ifndef LOCAL_STORAGE_SRC_MEMTABLE_H_
#define LOCAL_STORAGE_SRC_MEMTABLE_H_

#include <algorithm>
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
      : config(config), log(config.log_file_name),
        old_log(config.data_folder + "/" + config.olg_log_file_name),
        registry(registry) {}

  ~MemTable() override = default;

  bool put(const Entry &entry) {
    std::lock_guard lock_guard(mem_usage_mutex);

    log.putToLog(entry.key, entry.data);
    memory[entry.key] = entry.data;

    checkMemTableSize();

    return true;
  }

  bool get(Key key, std::string &data) {
    if (memory.find(key) != memory.end()) {
      data = memory[key];
      return true;
    }

    if (is_dumping && old_memory.find(key) != old_memory.end()) {
      data = old_memory[key];
      return true;
    }

    return false;
  }

  void start() override {
    old_log.start();
    auto [old_values, old_size] = old_log.readAll();
    memory = std::map(old_values.rbegin(), old_values.rend());


    log.start();

    auto [values, size] = log.readAll();
    memory.insert(values.rbegin(), values.rend());

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

  uint64_t getMemTableSize() const { return memory.size(); }

private:
  void dumpMemTable() {
    old_log.start();

    {
      std::lock_guard lock_guard(mem_usage_mutex);
      old_memory = memory;

      log.swap(old_log);

      memory.clear();
    }

    std::vector<Entry> entries;
    for (const auto &i : old_memory) {
      entries.push_back({i.first, i.second});
    }

    registry.createNewSSTableFromEntries(entries.begin(), entries.end());

    {
      std::lock_guard lock_guard(mem_usage_mutex);

      old_log.clear();

      is_dumping = false;
    }
  }

  bool checkMemTableSize() {
    if (getMemTableSize() > config.mem_table_dump_size && !is_dumping) {
      is_dumping = true;

      if (mem_table_dumper.joinable()) {
        mem_table_dumper.join();
      }

      mem_table_dumper = std::thread(&MemTable::dumpMemTable, this);
      return true;
    }
    return false;
  }

private:
  const Config &config;

  bool is_running = false;
  bool is_dumping = false;

  Log log;
  Log old_log;

  std::map<Key, std::string> memory;
  std::map<Key, std::string> old_memory;

  std::mutex mem_usage_mutex;
  std::thread mem_table_dumper;

  SSTableRegistry &registry;
};

#endif // LOCAL_STORAGE_SRC_MEMTABLE_H_
