//
// Created by pospelov on 09.12.2021.
//

#ifndef LOCAL_STORAGE_SRC_BYTE_IO_H_
#define LOCAL_STORAGE_SRC_BYTE_IO_H_

#include <fstream>
#include <iostream>
#include <string>

template <typename T> struct byte_writer {
  explicit byte_writer(const T &data) : _data(data) {}

  const T &_data;

  byte_writer(const byte_writer &) = delete;
  T &operator=(const T &) = delete;
};

template <typename T> struct byte_reader {
  explicit byte_reader(T &data) : _data(data) {}

  T &_data;

  byte_reader(const byte_reader &br) = delete;
  T &operator=(const T &br) = delete;
};

std::ostream &operator<<(std::ostream &, const byte_writer<uint64_t> &);
std::ostream &operator<<(std::ostream &, const byte_writer<std::string> &);
std::ostream &operator<<(std::ostream &, const byte_writer<bool> &);

std::istream &operator>>(std::istream &, const byte_reader<uint64_t> &);
std::istream &operator>>(std::istream &, const byte_reader<std::string> &);
std::istream &operator>>(std::istream &, const byte_reader<bool> &);

#endif // LOCAL_STORAGE_SRC_BYTE_IO_H_
