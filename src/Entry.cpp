//
// Created by pospelov on 09.12.2021.
//

#include "Entry.h"
#include "byteIO.h"

bool Entry::operator<(const Entry &rhs) const { return key < rhs.key; }
bool Entry::operator==(const Entry &rhs) const { return key == rhs.key; }

std::ofstream &operator<<(std::ofstream &os, const Entry &entry) {
  os << byte_writer(entry.key) << byte_writer(entry.data.size());
  os.write(entry.data.data(),
           static_cast<std::streamsize>(entry.data.size()));
  return os;
}

std::ifstream &operator>>(std::ifstream &is, Entry &entry) {
  size_t data_size;
  is >> byte_reader(entry.key) >> byte_reader(data_size);
  entry.data.resize(data_size);
  is.read(entry.data.data(), static_cast<std::streamsize>(data_size));
  return is;
}
