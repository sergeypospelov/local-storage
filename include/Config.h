//
// Created by pospelov on 09.12.2021.
//

#ifndef LOCAL_STORAGE_INCLUDE_CONFIG_H_
#define LOCAL_STORAGE_INCLUDE_CONFIG_H_

#include <iostream>
#include <string>
#include <utility>

struct Config {
  const std::string data_folder;

  const std::string data_file_name;
  const std::string log_file_name;

  const std::string sstable_prefix;

  const std::uint64_t mem_table_dump_size;

  const std::uint64_t sstable_skip_size;

  Config(std::string data_folder, std::string storage_file_name,
         std::string log_file_name, std::string sstable_prefix,
         std::uint64_t mem_table_dump_size, std::uint64_t sstable_skip_size)
      : data_folder(std::move(data_folder)),
        data_file_name(std::move(storage_file_name)),
        log_file_name(std::move(log_file_name)),
        sstable_prefix(std::move(sstable_prefix)),
        mem_table_dump_size(mem_table_dump_size),
        sstable_skip_size(sstable_skip_size) {}
};

#endif // LOCAL_STORAGE_INCLUDE_CONFIG_H_
