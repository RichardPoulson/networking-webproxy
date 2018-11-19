/*
 * webproxy.cpp
 *
 *  Created on: Nov 18, 2018
 *      Author: richard
 */

#include "webproxy.h"

#include <iostream>

WebProxy::WebProxy(int port_num, int timeout) {
	std::cout << "Constructor" << std::endl;
	this->sock_ = 0;
	this->port_number_ = 0;
	this->continue_ = true;
	this->reuse_socket_addr_ = 1;
	this->shared_resources_ = new struct webproxy_space::SharedResources();
	StartHTTPServices();
}

WebProxy::~WebProxy() {
	std::cout << "Destructor" << std::endl;
	close(this->sock_);
	delete(this->shared_resources_); // need to use delete, free() doesn't call destructor
	std::cout << "Server socket closed" << std::endl;
}

bool WebProxy::CreateBindSocket() {
	//  0= pick any protocol that socket type supports, can also use IPPROTO_TCP
	if ((this->sock_ = socket(AF_INET, SOCK_STREAM, 0))< 0) {
		perror("ERROR opening socket");
		return false;
	}
	//  Allows bind to reuse socket address (if supported)
	setsockopt(this->sock_, SOL_SOCKET, SO_REUSEADDR,
	    (const void *)&reuse_socket_addr_, sizeof(int));
	//  Define the server's Internet address
	bzero((char *) &server_addr_, sizeof(server_addr_));
	server_addr_.sin_family = AF_INET;
	server_addr_.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr_.sin_port = htons(port_number_);
	if (bind(sock_, (struct sockaddr *)&server_addr_, sizeof(server_addr_)) < 0) {
	    perror("ERROR on binding");
	    return false;
	}
	return true;
}

void WebProxy::StartHTTPServices() {
	pthread_mutex_lock(&this->shared_resources_->continue_mx); //== MUTEX
	std::cout << this->continue_ << std::endl;
	while (this->continue_ == true) {
		pthread_mutex_unlock(&this->shared_resources_->continue_mx); //== UNLOCK
		pthread_mutex_lock(&this->shared_resources_->continue_mx); //== MUTEX
	}
	pthread_mutex_unlock(&this->shared_resources_->continue_mx); //== UNLOCK
}
