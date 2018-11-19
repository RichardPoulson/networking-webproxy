#include <cstdlib>
#include <iostream>

#include "webproxy.h"

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cout << std::endl <<
      "Invalid number of additional arguments, please enter two numbers (port " <<
      "number, number threads) as argument." << std::endl;
      exit(EXIT_FAILURE);
  }
  WebProxy myWebProxy(std::atoi(argv[1]), std::atoi(argv[2]));
}
