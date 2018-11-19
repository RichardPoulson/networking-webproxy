/*
 * shared_resources.cpp
 *
 *  Created on: Nov 18, 2018
 *      Author: richard
 */

#include "webproxy_namespace.h"

namespace webproxy_space {

void SignalHandler(int sig)
{
  printf("User pressed Ctrl+C\n");
  exit(1);
}

SharedResources::SharedResources() {
	pthread_mutex_init(&this->file_mx, NULL);
	pthread_mutex_init(&this->map_mx, NULL);
	pthread_mutex_init(&this->queue_mx, NULL);
	pthread_mutex_init(&this->cout_mx, NULL);
	pthread_mutex_init(&this->continue_mx, NULL);
	this->regex_map["request line"] = std::regex(
			"^(GET|HEAD|POST) (\\/|(\\/.*)+\\.(html|txt|png|gif|jpg|css|js)) HTTP\\/\\d\\.\\d\r\n");
}

SharedResources::~SharedResources() {
	pthread_mutex_destroy(&this->file_mx);
	pthread_mutex_destroy(&this->map_mx);
	pthread_mutex_destroy(&this->queue_mx);
	pthread_mutex_destroy(&this->cout_mx);
	pthread_mutex_destroy(&this->continue_mx);
	std::cout << "PThread Mutexes destroyed properly" << std::endl;
}

} /* namespace webproxy_space */
