//
// Created by pospelov on 11.12.2021.
//

#ifndef LOCAL_STORAGE_INCLUDE_SSTABLEREGISTRY_H_
#define LOCAL_STORAGE_INCLUDE_SSTABLEREGISTRY_H_

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>

#include "IRunnable.h"
#include "SSTable.h"

std::string getTableName(const std::string &prefix, uint64_t id);

class SSTableRegistry : public IRunnable {
public:
  explicit SSTableRegistry(const Config &config) : config(config) {}

  template <class ForwardIterator>
  void createNewSSTableFromEntries(ForwardIterator first,
                                   ForwardIterator last) {
    size_t id = tables.size();

    std::string filename_prefix = getTableName(config.sstable_prefix, id);

    createSSTableFromEntries(first, last, filename_prefix);

    tables.emplace_back(filename_prefix, config.sstable_skip_size);
  }

  bool get(Key key, std::string &data) {
    for (auto it = tables.rbegin(); it != tables.rend(); it++) {
      if (it->get(key, data)) {
        return true;
      }
    }
    return false;
  }

  void start() override {
    initializeTables();

    is_running = true;
  }

  void stop() override { is_running = false; }

  bool isRunning() const override { return is_running; }

  size_t size() const { return tables.size(); }

private:
  void initializeTables() {
    std::string folder = config.data_folder;

    std::vector<std::string> paths;
    for (const auto &entry : std::filesystem::directory_iterator(folder)) {
      if (entry.path().string().starts_with(config.sstable_prefix) &&
          !entry.path().string().ends_with("idx.bin")) {
        paths.emplace_back(entry.path().string());
      }
    }
    std::sort(paths.begin(), paths.end());


    for (const auto &path : paths) {
      tables.emplace_back(path.substr(0, path.size() - 4), config.sstable_skip_size);
    }
  }

private:
  const Config &config;

  std::vector<SSTable> tables;

  bool is_running = false;
};

#endif // LOCAL_STORAGE_INCLUDE_SSTABLEREGISTRY_H_
