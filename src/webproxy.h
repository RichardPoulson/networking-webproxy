/*
 * webproxy.h
 *
 *  Created on: Nov 18, 2018
 *      Author: richard
 *
 *  https://www.ibm.com/support/knowledgecenter/en/ssw_ibm_i_73/rzab6/xnonblock.htm
 */

#ifndef NETWORKING_WEBPROXY_H_
#define NETWORKING_WEBPROXY_H_

#include <stddef.h> // NULL, nullptr_t
#include <stdio.h> // FILE, size_t, fopen, fclose, fread, fwrite,
#include <iostream> // cout
#include <fstream> // ifstream
#include <signal.h> // signal(int void (*func)(int))
#include <unistd.h>
#include <stdlib.h> //  exit, EXIT_FAILURE, EXIT_SUCCESS
#include <string.h> //  strlen, strcpy, strcat, strcmp
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h> //  socklen_t,
#include <netinet/in.h> //  sockaddr_in, INADDR_ANY,
#include <arpa/inet.h> //  htonl, htons, inet_ntoa,
#include <dirent.h> //  "Traverse directory", opendir, readdir,
#include <errno.h> //  "C Errors", errno
#include <regex> //  Regular expressions
#include <pthread.h> // process threads,
#include <ctime> //  time_t, tm,
#include <stack> //  stack of process threads
#include <sys/ioctl.h> // set socket to be nonbinding

#include "webproxy_namespace.h"

class WebProxy {
public:
	WebProxy(int port_num, int timeout = 30);
	virtual ~WebProxy();
	void ShutDownServer() { this->continue_ = false; }
private:
	int sock_; // file descriptor for server socket
	struct sockaddr_in server_addr_; // server's addr
	int port_number_; // port number of the server socket
	int on_; // used by setsockopt to tell it to reuse socket address
	bool continue_;
	struct webproxy_space::SharedResources * shared_resources_;
	std::time_t since_last_activity_;

	bool CreateBindSocket();
	void StartHTTPServices();
};

#endif // NETWORKING_WEBPROXY_H_
