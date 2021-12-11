//
// Created by pospelov on 09.12.2021.
//

#ifndef LOCAL_STORAGE_SRC_ENTRY_H_
#define LOCAL_STORAGE_SRC_ENTRY_H_

#include <ostream>
#include <vector>

using Key = uint64_t;

struct Entry {
  Key key;
  std::string data;

  bool operator<(const Entry &rhs) const;
  bool operator==(const Entry &rhs) const;

  friend std::ostream &operator<<(std::ostream &os, const Entry &entry);
  friend std::istream &operator>>(std::istream &os, Entry &entry);
};



#endif // LOCAL_STORAGE_SRC_ENTRY_H_
