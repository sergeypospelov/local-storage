//
// Created by pospelov on 05.12.2021.
//

#ifndef LOCAL_STORAGE_SRC_LSMSTORAGE_H_
#define LOCAL_STORAGE_SRC_LSMSTORAGE_H_

#include <utility>

#include "Entry.h"
#include "IRunnable.h"
#include "MemTable.h"
#include "SSTableRegistry.h"

class LsmStorage : public IRunnable {
public:
  explicit LsmStorage(const Config &config)
      : config(config), registry(config), mem_table(config, registry) {}

  bool put(Key key, const std::string &value) {
    mem_table.put({key, value});
    return true;
  }

  bool get(Key key, std::string &data) {
    if (mem_table.get(key, data)) {
      return true;
    }

    if (registry.get(key, data)) {
      return true;
    }

    return false;
  }

  void start() override {
    mem_table.start();
    registry.start();

    std::cerr << "OK: sstables=" << registry.size() << "\n";


    is_running = true;
  }

  void stop() override { is_running = false; }

  bool isRunning() const override { return is_running; }

  ~LsmStorage() override = default;

private:
  const Config &config;

  bool is_running = false;

  SSTableRegistry registry;
  MemTable mem_table;
};

#endif // LOCAL_STORAGE_SRC_LSMSTORAGE_H_
