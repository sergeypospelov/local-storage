//
// Created by pospelov on 09.12.2021.
//

#include <fstream>
#include <iostream>

#include "LsmStorage.h"

using namespace std::string_literals;

enum Cmd {
  EXIT, PUT, GET
};

int main() {
  Config config("data", "data/data", "data/log.bin", "data/sst", 256, 10);

  LsmStorage storage(config);
  storage.start();

  std::map<std::string, Cmd> string_to_cmd = {
      {"exit", Cmd::EXIT},
      {"+", Cmd::PUT},
      {"?", Cmd::GET}
  };

  bool is_running = true;
  while (is_running) {
    std::string cmd;
    std::cin >> cmd;
    switch (string_to_cmd[cmd]) {
    case Cmd::EXIT: {
      is_running = false;
      break;
    }
    case Cmd::PUT: {
      Key key;
      std::string value;
      std::cin >> key >> value;
      storage.put(key, value);
      break;
    }
    case Cmd::GET: {
      Key key;
      std::string value;
      std::cin >> key;
      if (storage.get(key, value)) {
        std::cout << value << std::endl;
      } else {
        std::cout << "No such value!" << std::endl;
      }
      break;
    }
    default:
      std::cout << "Unknown command!" << std::endl;
      break;
    }
  }

  return 0;
}