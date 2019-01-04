//============================================================================
// Name        : webproxy.h
// Author      : Richard Poulson
// Version     : 1.0
// Last edit   : 11/21/2018
//
// Description : Definition of buffer sizes and maximum number of pthreads,
//               definitions of helper functions DateTimeRFC and the pthread
//               function AcceptConnection.
//
// Much of the following code was inspired by examples at the following sites:
//     http://beej.us/guide/bgnet/html/single/bgnet.html
//     https://www.ibm.com/support/knowledgecenter/en/ssw_ibm_i_73/rzab6/xnonblock.htm
//============================================================================

#ifndef NETWORKING_WEBPROXY_H_
#define NETWORKING_WEBPROXY_H_

#define WEBPROXY_NUM_PTHREADS 8
#define WEBPROXY_BUFFER_SIZE 10485760

#include <stddef.h> // NULL, nullptr_t
#include <stdio.h> // FILE, size_t, fopen, fclose, fread, fwrite,
#include <iostream> // cout
#include <fstream> // ifstream
#include <signal.h> // signal(int void (*func)(int))
#include <unistd.h>
#include <stdlib.h> //  exit, EXIT_FAILURE, EXIT_SUCCESS
#include <string>
#include <string.h> //  strlen, strcpy, strcat, strcmp
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h> //  socklen_t,
#include <netinet/in.h> //  sockaddr_in, INADDR_ANY,
#include <arpa/inet.h> //  htonl, htons, inet_ntoa,
#include <dirent.h> //  "Traverse directory", opendir, readdir,
#include <cerrno> //  "C Errors", errno
#include <regex> //  Regular expressions
#include <pthread.h> // process threads,
#include <ctime> //  time_t, tm,
#include <stack> //  stack of process threads
#include <sys/ioctl.h> // set socket to be nonbinding
#include <queue>
#include <map> // map of string to regex
#include <unordered_set> // unordered set of blacklisted sites

// PThread function
void * AcceptConnection(void * shared_resources);
// Process HTTP request message, assigning values to struct RequestMessage
bool ProcessHTTPRequest(char * buf, struct RequestMessage * req);
// send HTTP/1.1 400 Bad Request\r\n\r\n\r\n
void SendBadRequest(int sock);
// keeps sending until all bytes sent
ssize_t SendWholeMessage(int sock, char * buf, int buf_size);

// struct
struct CachedPage {
	pthread_mutex_t file_mx;
	FILE * file;
	char filename[256];
	long file_size = 0;
	time_t created;
	CachedPage();
	virtual ~CachedPage();
};
// data structure dealing with HTTP request messages
struct RequestMessage
{
	int clients_sd;
	std::string method; // GET,HEAD,POST
	std::string request; // /,
	std::string request_line;
	std::string http; // e.g. HTTP/1.1
	std::string host_name; // e.g. localhost or
	std::string host_port; // e.g. localhost or
	std::string connection; // keep-alive, close
	struct addrinfo host_addrinfo, * host_res;
	RequestMessage();
	virtual ~RequestMessage();
};
// struct designed to be used between the WebProxy object and pthreads
struct SharedResources {
	int ttl; // time-to-live for the cached pages
	int num_cached_pages = 0;
	pthread_mutex_t file_mx, map_mx, queue_mx, cout_mx, continue_mx;
	std::map<std::string, std::regex> regex_map; // map from string to regex
	std::map<std::string, struct CachedPage *> cached_page_map;
	std::map<std::string, struct addrinfo *> cached_addrinfo_map;
	std::queue<struct RequestMessage> request_queue;//client_queue;
	std::unordered_set<std::string> blacklist_sites;
	SharedResources();
	virtual ~SharedResources();
};

// WebProxy constructor takes two arguments (see below for default values).
// The first argument is
class WebProxy {
public:
	WebProxy(char * port_num, int timeout = 60, int ttl = 5, std::string http_addr = "localhost");
	virtual ~WebProxy();
private:
	int ttl_; // time-to-live for the cached pages
	pthread_t proxy_connections[WEBPROXY_NUM_PTHREADS];
	pthread_attr_t pthread_attr; // attributes for the pthreads
  void * status;
	struct timeval timeout_; // timeout of server's listen socket
	fd_set master_set_, working_set_; // file descriptor sets, used with select()
	int listen_sd_, max_sd_, new_sd_; // listen/max/new socket descriptors
	std::string http_address_str_, http_port_str_;
	struct sockaddr_in webproxy_addr_;
	int port_number_; // port number of the server socket
	struct SharedResources * shared_;
	int on_; // used by setsockopt to tell it to reuse socket address
	int off_; // used by setsockopt to tell it to reuse socket address
	bool CreateBindSocket();
	void StartHTTPServices();
};

#endif // NETWORKING_WEBPROXY_H_
