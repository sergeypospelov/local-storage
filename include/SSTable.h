//
// Created by pospelov on 05.12.2021.
//

#ifndef LOCAL_STORAGE_SRC_SSTABLE_H_
#define LOCAL_STORAGE_SRC_SSTABLE_H_

#include <string>
#include <utility>

#include "IRunnable.h"
#include "Config.h"

class SSTable : public IRunnable {
public:
  explicit SSTable(Config config, std::uint64_t id)
      : config(std::move(config)), id(id) {
  }

  void start() override {
    is_running = true;
  }
  void stop() override {
    is_running = false;
  }
  bool isRunning() const override {
    return is_running;
  }

private:
  const Config config;
  const uint64_t id;

  bool is_running = false;
};

#endif // LOCAL_STORAGE_SRC_SSTABLE_H_
