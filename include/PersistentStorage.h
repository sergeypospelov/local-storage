//
// Created by pospelov on 03.12.2021.
//

#ifndef LOCAL_STORAGE__PERSISTENTSTORAGE_HPP_
#define LOCAL_STORAGE__PERSISTENTSTORAGE_HPP_

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Config.h"

class PersistentStorage {
public:
  explicit PersistentStorage(Config config) : config(std::move(config)) {
    std::cerr << "Reading data...\n";
    read_all();
    std::cerr << "Success. Entries: " << storage.size()
              << "\nDumping data...\n";
    dump_storage();
    std::cerr << "Success!\n\n";

    // clear content of a file
    out_log.open(config.log_file_name, std::ofstream::trunc);
  }

  bool find(const std::string &key) {
    return storage.find(key) != storage.end();
  }

  uint64_t get(const std::string &key) {
    if (!find(key)) {
      throw std::logic_error("No such element: " + key);
    }
    return storage[key];
  }

  bool put(const std::string &key, uint64_t value) {
    add_to_log(key, value);

    storage[key] = value;
    return true;
  }

private:
  void add_to_log(const std::string &key, uint64_t value) {
    out_log << key << " " << value << "\n";
    out_log.flush();
  }

  void read_all() {
    std::ifstream in_data(config.data_file_name);
    std::ifstream in_log(config.log_file_name);

    std::string key;
    uint64_t value;
    while (in_data >> key >> value) {
      storage[key] = value;
    }

    while (in_log >> key >> value) {
      storage[key] = value;
    }

    in_data.close();
    in_log.close();
  }

  void dump_storage() {
    std::ofstream out_data;
    out_data.open(config.data_file_name, std::ofstream::trunc);
    for (auto &[key, value] : storage) {
      out_data << key << " " << value << "\n";
    }
    out_data.close();
  }

private:
  Config config;

  std::ofstream out_log;

  std::unordered_map<std::string, uint64_t> storage;
};

#endif // LOCAL_STORAGE__PERSISTENTSTORAGE_HPP_
