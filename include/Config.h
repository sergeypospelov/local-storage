//
// Created by pospelov on 09.12.2021.
//

#ifndef LOCAL_STORAGE_INCLUDE_CONFIG_H_
#define LOCAL_STORAGE_INCLUDE_CONFIG_H_

#include <string>
#include <utility>
#include <iostream>

struct Config {
  const std::string data_file_name;
  const std::string log_file_name;

  const std::string sstable_prefix;
  const std::string sstable_index_prefix;

  Config(std::string storage_file_name, std::string log_file_name,
         std::string sstable_prefix,
         std::string sstable_index_prefix)
      : data_file_name(std::move(storage_file_name)),
        log_file_name(std::move(log_file_name)),
        sstable_prefix(std::move(sstable_prefix)),
        sstable_index_prefix(std::move(sstable_index_prefix)) {
  }

};

#endif // LOCAL_STORAGE_INCLUDE_CONFIG_H_
