//============================================================================
// Name        : pa3.cpp
// Author      : Richard Poulson
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <csignal>

#include "webproxy.h"
#include "webproxy_namespace.h"

int main() {
	signal(SIGINT, webproxy_space::SignalHandler);
	std::cout << "!!!Hello World!!!" << std::endl; // prints !!!Hello World!!!
	WebProxy my_web_proxy(8008, 60);
	return 0;
}
