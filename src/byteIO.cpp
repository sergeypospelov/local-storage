#include "byteIO.h"

/* WRITING */

static const int BYTE_SIZE = 8;

std::ostream &operator <<(std::ostream &os, const byte_writer<uint64_t> &bw) {
  for (size_t b = 0; b < sizeof(uint64_t); b++) {
    os.put((char)((bw._data >> (BYTE_SIZE * b)) & (0xff)));
  }
  return os;
}

std::ostream &operator <<(std::ostream &os, const byte_writer<std::string> &bw) {
  for (char i : bw._data) {
    os.put(i);
  }
  os.put(0);
  return os;
}

std::ostream &operator <<(std::ostream &os, const byte_writer<bool> &bw) {
  for (size_t b = 0; b < sizeof(int); b += BYTE_SIZE) {
    os.put((char)(bw._data));
  }
  return os;
}


/* READING */

std::istream &operator >>(std::istream &is, const byte_reader<uint64_t> &br) {
  br._data = 0;
  for (size_t b = 0; b < sizeof(uint64_t); b++) {
    char c;
    is.get(c);
    br._data |= ((uint64_t)((unsigned char)c) << (b * BYTE_SIZE));
  }
  return is;
}

std::istream &operator >>(std::istream &is, const byte_reader<std::string> &br) {
  br._data = "";
  char c;
  while (is.get(c) && c != '\0') {
    br._data += c;
  }
  return is;
}

std::istream &operator >>(std::istream &is, const byte_reader<bool> &br) {
  char c;
  is.get(c);
  br._data = c;
  return is;
}