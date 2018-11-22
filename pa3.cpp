//============================================================================
// Name        : pa3.cpp
// Author      : Richard Poulson
// Version     : 1.0
// Last edit   : 11/21/2018
//
// Description : This program creates a WebProxy object that starts a web
//               proxy service as soon as the object's constructor executes.
//               The proxy takes two arguments for its constructor, a port
//               number from which it will offer the proxy service, and the
//               timeout value for the socket listening for clients.
//
//               See the webproxy.h file for the definitions of both the
//               object's/pthreads' buffer sizes and the maximum number of
//               pthreads allowed.
//============================================================================

#include <csignal>

#include "webproxy.h"

int main(int argc, char **argv) {
	if (argc != 3) {
	    std::cout << argc << std::endl;
	    std::cout <<
	      "Invalid number of additional arguments, please enter two integers" <<
	      " <port number> <timeout> as arguments." << std::endl;
	      //exit(EXIT_FAILURE);
	  }
	std::cout << "Starting WebProxy on port \"" << atoi(argv[1]) << "\" with a timeout of \"" << atoi(argv[2]) <<
			"\" seconds." << std::endl;
	WebProxy my_web_proxy(argv[1], atoi(argv[2]));
	return 0;
}
