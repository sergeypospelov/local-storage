//
// Created by pospelov on 11.12.2021.
//

#include "SSTableRegistry.h"
#include <iomanip>

std::string getTableName(const std::string &prefix, uint64_t id) {
  std::stringstream ss;
//  ss << std::setw(3) << std::setfill('0') << id;
  ss << id;
  return prefix + ss.str();
}
