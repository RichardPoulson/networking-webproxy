/*
 * shared_resources.h
 *
 *  Created on: Nov 18, 2018
 *      Author: richard
 */

#ifndef NETWORKING_WEBPROXY_NAMESPACE_H_
#define NETWORKING_WEBPROXY_NAMESPACE_H_

#include <iostream>
#include <pthread.h> // process threads,
#include <map> // map from string to regex
#include <string> // ""
#include <regex> // ""
#include <queue> // queue of client sockets that need servicing

namespace webproxy_space {

void SignalHandler(int sig);

struct SharedResources {
	pthread_mutex_t file_mx, map_mx, queue_mx, cout_mx, continue_mx;
	std::map<std::string, std::regex> regex_map; // map from string to regex
	std::queue<int> client_queue;
	SharedResources();
	virtual ~SharedResources();
};

} /* namespace webproxy_space */

#endif /* NETWORKING_WEBPROXY_NAMESPACE_H_ */
