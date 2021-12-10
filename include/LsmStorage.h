//
// Created by pospelov on 05.12.2021.
//

#ifndef LOCAL_STORAGE_SRC_LSMSTORAGE_H_
#define LOCAL_STORAGE_SRC_LSMSTORAGE_H_

#include <utility>

#include "Entry.h"
#include "IRunnable.h"
#include "MemTable.h"
#include "SSTable.h"

class LsmStorage : public IRunnable {
public:
  explicit LsmStorage(const Config &config)
      : config(config), mem_table(config) {}

  bool put(Key key, const std::string &value) {
    mem_table.put({key, value});
    return true;
  }

  bool get(Key key, std::string &data) { return mem_table.get(key, data); }

  void start() override {
    mem_table.start();
    is_running = true;
  }

  void stop() override { is_running = false; }

  bool isRunning() const override { return is_running; }

  ~LsmStorage() override = default;

private:
  const Config config;

  bool is_running = false;

  MemTable mem_table;
};

#endif // LOCAL_STORAGE_SRC_LSMSTORAGE_H_
