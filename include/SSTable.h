//
// Created by pospelov on 05.12.2021.
//

#ifndef LOCAL_STORAGE_SRC_SSTABLE_H_
#define LOCAL_STORAGE_SRC_SSTABLE_H_

#include <cassert>
#include <map>
#include <sstream>
#include <string>
#include <utility>

#include "Config.h"
#include "Entry.h"
#include "byteIO.h"

struct IndexEntry {
  Key key;
  uint64_t offset;
};

static std::string prefixToFile(const std::string &prefix) {
  return prefix + ".bin";
}

static std::string prefixToIndexFile(const std::string &prefix) {
  return prefix + "_idx.bin";
}

class SSTable {
public:
  explicit SSTable(const std::string &prefix, size_t skip_size)
      : prefix(prefix), sst(prefixToFile(prefix), std::ios::binary),
        sst_idx(prefixToIndexFile(prefix), std::ios::binary) {
    readAll(skip_size);
  }

  void readAll(size_t skip_size) {
    assert(sst_idx.is_open() && sst.is_open());

    Key key;
    uint64_t offset;
    while (sst_idx >> byte_reader(key) >> byte_reader(offset)) {
      key_to_idx_offset[key] = sst_idx.tellg();
      sst_idx.seekg((skip_size - 1) * (sizeof(key) + sizeof(offset)),
                    std::ios::cur);
    }
    sst_idx.clear();
    sst_idx.seekg(0, std::ios::end);
    sst_idx.seekg(-static_cast<long>(sizeof(key)) - sizeof(offset),
                  std::ios::end);
    if (sst_idx >> byte_reader(key) >> byte_reader(offset)) {
      key_to_idx_offset[key] = sst_idx.tellg();
    }
  }

  bool get(Key key, std::string &data) {
    uint64_t offset;
    if (!getOffset(key, offset)) {
      return false;
    }

    sst.seekg(offset, std::ios::beg);

    Entry entry;
    sst >> entry;
    data = entry.data;
    return true;
  }

  const std::string &name() const { return prefix; }

private:
  bool getOffset(Key key, uint64_t &offset) {
    auto it_low = key_to_idx_offset.upper_bound(key);
    auto it_high = key_to_idx_offset.lower_bound(key);

    if (it_low == key_to_idx_offset.begin() ||
        it_high == key_to_idx_offset.end()) {
      return false;
    }
    it_low--;

    std::streamsize pos_low = it_low->second - sizeof(key) - sizeof(uint64_t);
    std::streamsize pos_high = it_high->second;
    std::streamsize length = pos_high - pos_low;

    std::vector<char> buffer(length);

    sst_idx.clear();
    sst_idx.seekg(pos_low, std::ios::beg);
    sst_idx.read(buffer.data(), length);

    std::stringstream local_stream;
    local_stream.rdbuf()->pubsetbuf(buffer.data(), length);

    IndexEntry index_entry{};
    while (local_stream >> byte_reader(index_entry.key) >>
           byte_reader(index_entry.offset)) {
      if (index_entry.key == key) {
        offset = index_entry.offset;
        return true;
      }
    }

    return false;
  };

private:
  const std::string prefix;

  std::ifstream sst;
  std::ifstream sst_idx;

  std::map<Key, uint64_t> key_to_idx_offset;
};

template <class ForwardIterator>
void createSSTableFromEntries(ForwardIterator first, ForwardIterator last,
                                 const std::string &filename_prefix) {
  std::ofstream out(prefixToFile(filename_prefix), std::ios::binary);
  std::ofstream out_idx(prefixToIndexFile(filename_prefix), std::ios::binary);

  while (first != last) {
    Entry &entry = *first;

    out_idx << byte_writer(entry.key)
            << byte_writer(static_cast<uint64_t>(out.tellp()));
    out << entry;
    first++;
  }

  out.close();
  out_idx.close();
}

#endif // LOCAL_STORAGE_SRC_SSTABLE_H_
