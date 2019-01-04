//============================================================================
// Name        : webproxy.cpp
// Author      : Richard Poulson
// Version     : 1.0
// Last edit   : 11/21/2018
//
// Description :
//
// The following sites/examples examples contributed significantly:
//     https://www.ibm.com/developerworks/community/blogs/IMSupport/entry/WHY_DOES_SEND_RETURN_EAGAIN_EWOULDBLOCK?lang=en
//     https://www.geeksforgeeks.org/socket-programming-cc/
//============================================================================

#include "webproxy.h"

#include <iostream>

// PThread function
void * AcceptConnection(void * shared_resources) {
	/*
	// cast shared resources so we can use
	struct SharedResources * shared = (struct SharedResources*)shared_resources;
	char * buffer = (char*) malloc (WEBPROXY_BUFFER_SIZE);
	int bytes_received, bytes_sent;
	bool page_cached, cached_page_old; // Is the requested page cached?  Is the cached page old?
	std::map<std::string, struct CachedPage *>::iterator cached_iter;
	std::map<std::string, struct addrinfo *>::iterator addrinfo_iter;
	struct CachedPage * cached_point;
	struct addrinfo hints, *results, *rp; // used with getaddrinfo()
	int i; // iterator for loop in StartHTTPServices()
	int http_serv_sd;

	RequestMessage request = shared->request_queue.front();
	shared->request_queue.pop();

	std::cout << "(" << request.clients_sd << ") [" << request.host_name << ":" <<
			request.host_port << "]  " << request.request_line << std::endl;
	cached_iter = shared->cached_page_map.find(request.request_line);
	// PAGE NOT CACHED page_cached, cached_page_old
	if (cached_iter == shared->cached_page_map.end()) {
		page_cached = false;
	}
	else {
		page_cached = true;
		if (difftime(time(NULL), cached_iter->second->created) < shared->ttl) {
			cached_page_old = false;
		}
		else {
			cached_page_old = true;
		}
	}
	std::cout << "Finished determining cache status of page\n";
	// If this expression evaluates to true, need to write/update to cache
	if ((page_cached == false) ||
			((page_cached == true) && (cached_page_old == true))) {
		addrinfo_iter = shared->cached_addrinfo_map.find(request.host_name + ":" + request.host_port);
		// addrinfo cached for host name and port
		if (addrinfo_iter != shared->cached_addrinfo_map.end()) {
			std::cout << ">> using cached addrinfo for \"" << request.host_name <<
					":"<< request.host_port << "\"" << std::endl;
			if ((http_serv_sd = socket(addrinfo_iter->second->ai_family, addrinfo_iter->second->ai_socktype,
					addrinfo_iter->second->ai_protocol)) == -1) {
				perror("socket() failed");
			}
			if (connect(http_serv_sd, addrinfo_iter->second->ai_addr,
					addrinfo_iter->second->ai_addrlen) == -1) {
				perror("connect() failed");
			}
			std::cout << "Finished connecting to cached addrinfo\n";
		}
		// addrinfo not cached for host name and port
		else {
			std::cout << "Need to figure out addrinfo from scratch\n";
			bzero(&hints, sizeof(hints));
			hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
			hints.ai_socktype = SOCK_STREAM; // Datagram socket
			hints.ai_flags = 0;
			hints.ai_protocol = 0;          // Any protocol
			/// hints specifies what the parameters for the search are, &result points to a list of addrinfo's
			if (getaddrinfo(request.host_name.c_str(), request.host_port.c_str(), &hints, &results) != 0) {
				perror("getaddrinfo() failed");
			}
			// go through all possible
			for (rp = results; rp != NULL; rp = rp->ai_next) {
				http_serv_sd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
				if (http_serv_sd == -1)
					continue;
				if (connect(http_serv_sd, rp->ai_addr, rp->ai_addrlen) != -1)
					break; // success
				close(http_serv_sd);
			}
			if (rp == NULL) { // no address succeeded
				std::cout << "Could not connect to host name and port" << std::endl;
			}
			//freeaddrinfo(results);
			shared->cached_addrinfo_map[request.host_name + ":" + request.host_port] = rp;
			std::cout << "Figured out addrinfo\n";
		}
		//if (write(http_serv_sd, buffer, bytes_received) != bytes_received) {
		if(SendWholeMessage(http_serv_sd, (char *) request.request.c_str(), sizeof request.request.c_str()) != sizeof request.request.c_str()) {
				fprintf(stderr, "partial/failed write\n");
				exit(EXIT_FAILURE);
		}
		std::cout << "Sent request to HTTP server\n";
		std::cout << request.request << std::endl;
		bzero(buffer, WEBPROXY_BUFFER_SIZE);
		bytes_received = recv(http_serv_sd, buffer, WEBPROXY_BUFFER_SIZE, 0);
		if (bytes_received == -1) {
			perror("read");
		}
		close(http_serv_sd);
		std::cout << "Received response from HTTP server\n";
		shared->cached_page_map[request.request_line] = new CachedPage();
		cached_point = shared->cached_page_map[request.request_line];
		pthread_mutex_lock(&cached_point->file_mx); // MUTEX lock file_mx
		std::cout << "<< caching page for \"" << request.request_line << "\"" << std::endl;
		time(&cached_point->created);
		strcat (cached_point->filename,"cache/");
		strcat (cached_point->filename, std::to_string(shared->num_cached_pages++).c_str());
		strcat (cached_point->filename,".bin");
		cached_point->file = fopen(cached_point->filename, "wb");
		if (cached_point->file != NULL) {
			fwrite(buffer, bytes_received, 1, cached_point->file);
			//fputs(request.request_line.c_str(),cached_point->file);
			fclose(cached_point->file);
		}
		cached_point->file_size = (long) bytes_received;
		pthread_mutex_unlock(&cached_point->file_mx); // MUTEX unlock file_mx
		//write(i, buffer, bytes_received); // after caching page, send to client
		SendWholeMessage(request.clients_sd, buffer, bytes_received); // after caching page, send to client
	}
	else { // PAGE CACHED,
		pthread_mutex_lock(&cached_iter->second->file_mx); // MUTEX lock file_mx
		std::cout << ">> using cached page for \"" << request.request_line << "\"" << std::endl;
		bzero(buffer, WEBPROXY_BUFFER_SIZE);
		cached_iter->second->file = fopen(cached_iter->second->filename, "rb");
		fread (buffer,1,cached_iter->second->file_size,cached_iter->second->file);
		fclose(cached_iter->second->file);
		SendWholeMessage(request.clients_sd, buffer, cached_iter->second->file_size);
		pthread_mutex_unlock(&cached_iter->second->file_mx); //== UNLOCK
	}
	close(request.clients_sd);
	delete [] buffer;
	std::cout << "Exiting pthread\n";
  pthread_exit(NULL);
	//*/
}

bool ProcessHTTPRequest(char * buf, struct RequestMessage * req) {
	std::cmatch cmatch1, cmatch2; // first match with a line, then with what you're looking for
	struct addrinfo hints, *results, *rp;

	std::regex_search (buf, cmatch1, std::regex("Host: .+"));
	std::regex_search (cmatch1.str().c_str(), cmatch2, std::regex("(?![Host: ])[^:]*"));
	req->host_name = cmatch2.str();
	// Had to make custom regex to separate port number from address
	std::regex_search (cmatch1.str().c_str(), cmatch2, std::regex("(?![Host: " + req->host_name + ":])[^:]*"));
	req->host_port = cmatch2.str();
	if (req->host_port == "") { req->host_port = "80"; } // if not defined, assume port 80
	std::regex_search (buf, cmatch1, std::regex("^(GET|HEAD|POST|CONNECT).*"));
	req->request_line = cmatch1.str().c_str();
	req->request = buf; // copy entire request
	std::regex_search (cmatch1.str().c_str(), cmatch2, std::regex("(GET|HEAD|POST|CONNECT)"));
	req->method = cmatch2.str();
	if (req->method.compare("GET") != 0) {
		return false;
	}
	return true;
}

void SendBadRequest(int sock) {
	char bad_request[] = "HTTP/1.1 400 Bad Request\r\n";
	strcat(bad_request, "Connection: close\r\n\r\n\r\n");
	if (SendWholeMessage(sock, bad_request, sizeof bad_request) < 0) {
		perror("send() failed");
	}
}
//*/

ssize_t SendWholeMessage(int sock, char * buf, int buf_size) {
	// http://beej.us/guide/bgnet/html/single/bgnet.html#sendall
	ssize_t total_sent = 0;        // how many bytes we've sent
  ssize_t bytes_left = buf_size; // how many we have left to send
  ssize_t bytes_sent;

  while(total_sent < buf_size) {
		bytes_sent = send(sock, buf + total_sent, bytes_left, 0);
    if (bytes_sent == -1) {
			perror("send() failed");
			break;
		}
    total_sent += bytes_sent;
    bytes_left -= bytes_sent;
  }
	return total_sent;
}
CachedPage::CachedPage() {
	this->file = nullptr;
	bzero(this->filename, sizeof this->filename);
	pthread_mutex_init(&this->file_mx, NULL);
}
CachedPage::~CachedPage() {
	pthread_mutex_destroy(&this->file_mx);
	fclose(this->file);
	if (remove(this->filename) != 0) {
		perror("remove() failed");
	}
}

RequestMessage::RequestMessage() {
	bzero(&this->host_addrinfo, sizeof this->host_addrinfo);
}
RequestMessage::~RequestMessage() {}

SharedResources::SharedResources() {
	pthread_mutex_init(&this->file_mx, NULL);
	pthread_mutex_init(&this->map_mx, NULL);
	pthread_mutex_init(&this->queue_mx, NULL);
	pthread_mutex_init(&this->cout_mx, NULL);
	pthread_mutex_init(&this->continue_mx, NULL);
	this->regex_map["request line"] = std::regex(
			"^(GET|HEAD|POST|CONNECT).*");
	this->regex_map["method"] = std::regex("(GET|HEAD|POST|CONNECT)"); // Method
	this->regex_map["http"] = std::regex("HTTP\\/\\d\\.\\d"); // HTTP Version
	this->regex_map["connection line"] = std::regex("Connection: (keep-alive|close)\r\n"); // Connection: keep-alive
	this->regex_map["connection"] = std::regex("(keep-alive|close)"); // Connection: keep-alive
	this->regex_map["headers"] = std::regex("HTTP.+\r\n(.+\r\n)+\r\n"); // Connection: keep-alive
	this->regex_map["host line"] = std::regex("Host: .+"); // Connection: keep-alive
	this->regex_map["host name"] = std::regex("(?![Host: ])[^:]*"); // Connection: keep-alive
	this->regex_map["host port"] = std::regex("(?![:]).*"); // Connection: keep-alive
}

SharedResources::~SharedResources() {
	pthread_mutex_destroy(&this->file_mx);
	pthread_mutex_destroy(&this->map_mx);
	pthread_mutex_destroy(&this->queue_mx);
	pthread_mutex_destroy(&this->cout_mx);
	pthread_mutex_destroy(&this->continue_mx);
	for (std::map<std::string, struct CachedPage *>::iterator it =
			cached_page_map.begin(); it != cached_page_map.end(); ++it) {
		delete(it->second);
		std::cout << "DELETE cached page: \"" << it->first << "\"" << std::endl;
	}
	std::cout << "~ SharedResources destructor ~" << std::endl;
}
//
WebProxy::WebProxy(char * port_num, int timeout, int ttl, std::string http_addr) {
	this->ttl_ = ttl;
	this->port_number_ = atoi(port_num);
	this->http_port_str_ = port_num;
	this->http_address_str_ = http_addr;
	this->timeout_.tv_sec = timeout; // for listening socket (in seconds)
	this->timeout_.tv_usec = 0;
	this->shared_ = new struct SharedResources();
	this->on_ = 1; // used by setsockopt to tell it to reuse socket address
	this->off_ = 0; // used to turn off setsockopt flags
	this->shared_->ttl = this->ttl_;
	// Initialize and set thread joinable
  pthread_attr_init(&pthread_attr);
  pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_JOINABLE);
	FILE * pFile;
  char mystring [100];
	std::ifstream infile("docs/blacklist.txt");
  std::string line;
  while (std::getline(infile, line)) {
		shared_->blacklist_sites.insert(line);
	}
	infile.close();
	// upon creation, WebProxy object immediately creates/binds a listening
	// socket, then loops around using select() to find sockets trying to
	// communicate with it.
	CreateBindSocket();
	StartHTTPServices();
}

WebProxy::~WebProxy() {
	pthread_attr_destroy(&pthread_attr);
	delete(this->shared_);
	std::cout << "~ WebProxy destructor ~" << std::endl;
}

bool WebProxy::CreateBindSocket() {
	//  0= pick any protocol that socket type supports, can also use IPPROTO_TCP
	if ((this->listen_sd_ = socket(AF_INET, SOCK_STREAM, 0))< 0) {
		perror("ERROR opening socket");
		return false;
	}
	//  Allows bind to reuse socket address (if supported)
	if (setsockopt(this->listen_sd_, SOL_SOCKET, SO_REUSEADDR,
	    (const void *)&on_, sizeof(int)) < 0) {
		perror("setsockopt() failed");
		close(listen_sd_);
		return false;
	}
	// set listen socket to be non-binding
	if (ioctl(listen_sd_, FIONBIO, (char *)&on_) < 0) {
		perror("ioctl() failed");
		close(listen_sd_);
		return false;
	}
	//  Define the server's Internet address
	bzero((char *) &webproxy_addr_, sizeof(webproxy_addr_));
	webproxy_addr_.sin_family = AF_INET;
	webproxy_addr_.sin_addr.s_addr = htonl(INADDR_ANY);
	webproxy_addr_.sin_port = htons(port_number_);
	if (bind(listen_sd_, (struct sockaddr *)&webproxy_addr_, sizeof(webproxy_addr_)) < 0) {
	    perror("ERROR on binding");
	    close(listen_sd_);
	    return false;
	}
	// set the listen back log
	if (listen(listen_sd_, 32) < 0) {
		perror("listen() failed");
		close(listen_sd_);
		return false;
	}
	// initialize master file-descriptor set
	FD_ZERO(&master_set_);
	max_sd_ = listen_sd_;
	//FD_SET(listen_sd_, &master_set_);
	FD_SET(listen_sd_, &master_set_);
	return true;
}

void WebProxy::StartHTTPServices() {
	bool continue_servicing = true;
	bool page_cached, cached_page_old; // Is the requested page cached?  Is the cached page old?
	long file_pos = 0;
	std::map<std::string, struct CachedPage *>::iterator cached_iter;
	std::map<std::string, struct addrinfo *>::iterator addrinfo_iter;
	std::unordered_set<std::string>::iterator blacklist_iter;
	struct CachedPage * cached_point;
	struct RequestMessage request;
	int i; // iterator for loop in StartHTTPServices()
	int http_serv_sd;
	int num_sockets_ready; // keeps track of how many socket descriptors are readable
	ssize_t bytes_received;
	struct sockaddr_in client_address; // client addr
	struct addrinfo hints, *results, *rp; // used with getaddrinfo()
	socklen_t addr_length;
	char * buffer = (char*) malloc (WEBPROXY_BUFFER_SIZE);
	char * error_message = "ERROR 403 Forbidden\r\n\r\n\r\n";

	while (continue_servicing) {
		// copy master set to working set
		memcpy(&working_set_, &master_set_, sizeof(master_set_));
		// return from select when descriptors readable or timeout occurred
		num_sockets_ready = select(max_sd_+1, &working_set_, NULL, NULL, &timeout_);
		if (num_sockets_ready < 0) { // ERROR
			perror("select() failed");
			continue_servicing = false;
		}
		else if (num_sockets_ready == 0) { // TIMEOUT
			std::cout << "~ Timeout occurred ~" << std::endl;
			continue_servicing = false;
		}
		else { // 1 or more socket descriptors are readable
			for (i=0; i <= max_sd_; ++i) {
				if (FD_ISSET(i, &working_set_)) { // read data from them
					if (i == listen_sd_) { // SERVER SOCKET
						if ((new_sd_ = accept(listen_sd_,
									(struct sockaddr *)&client_address, &addr_length)) < 0) {
							perror("accept() failed.");
						}
						std::cout << "+ LISTENING socket added (" << new_sd_ << ")" << std::endl;
						FD_SET(new_sd_, &master_set_);
						if (new_sd_ > max_sd_) {
							max_sd_ = new_sd_;
						}
					}
					else { // socket other than listening socket wants to send data
						if ((bytes_received = recv(i, buffer, WEBPROXY_BUFFER_SIZE, 0)) < 0) {
							if ((errno != EWOULDBLOCK) && (errno != EAGAIN)) { // ERROR
								perror("recv() failed");
								continue_servicing = false;
							}
						}
						else if (bytes_received == 0) { // Client closed connection
							std::cout << "- closing client socket (" << i << ")" << std::endl;
							close(i);
							FD_CLR(i, &master_set_);
							if (i == max_sd_) {
								if (FD_ISSET(max_sd_, &master_set_) == 0) {
									max_sd_ = max_sd_ - 1;
								}
							}
						}
						else { // bytes_received  > 0
							if (ProcessHTTPRequest(buffer, &request) != true) {
								SendBadRequest(i);
								std::cout << "! client " << i << ": bad request: \"" <<
										request.request_line << "\"" << std::endl;
								continue;
							}
							request.clients_sd = i;
							/*
							for (blacklist_iter = shared_->blacklist_sites.begin();
							blacklist_iter != shared_->blacklist_sites.end(); blacklist_iter++) {
								if (request.request_line.find(blacklist_iter) != std::string::npos) {
									SendWholeMessage(i, error_message, sizeof error_message);
									std::cout << "FORBIDDEN: \"" << request.request_line << "\"" << std::endl;
									continue;
								}
							}
							*/
							//shared_->request_queue.push(request);
							//pthread_create(&proxy_connections[0], &pthread_attr, AcceptConnection, shared_);
					    //pthread_join(proxy_connections[0],NULL); // thread returns to program
							//*/
							std::cout << "(" << i << ") [" << request.host_name << ":" <<
									request.host_port << "]  " << request.request_line << std::endl;
							cached_iter = shared_->cached_page_map.find(request.request_line);
							// PAGE NOT CACHED page_cached, cached_page_old
							if (cached_iter == shared_->cached_page_map.end()) {
								page_cached = false;
							}
							else {
								page_cached = true;
								if (difftime(time(NULL), cached_iter->second->created) < this->ttl_) {
									cached_page_old = false;
								}
								else {
									cached_page_old = true;
								}
							}
							// If this expression evaluates to true, need to write/update to cache
							if ((page_cached == false) ||
									((page_cached == true) && (cached_page_old == true))) {
								addrinfo_iter = shared_->cached_addrinfo_map.find(request.host_name + ":" + request.host_port);
								// addrinfo cached for host name and port
								if (addrinfo_iter != shared_->cached_addrinfo_map.end()) {
									std::cout << ">> using cached addrinfo for \"" << request.host_name <<
											":"<< request.host_port << "\"" << std::endl;
									if ((http_serv_sd = socket(addrinfo_iter->second->ai_family, addrinfo_iter->second->ai_socktype,
											addrinfo_iter->second->ai_protocol)) == -1) {
										perror("socket() failed");
									}
									if (connect(http_serv_sd, addrinfo_iter->second->ai_addr,
											addrinfo_iter->second->ai_addrlen) == -1) {
										perror("connect() failed");
									}
								}
								// addrinfo not cached for host name and port
								else {
									bzero(&hints, sizeof(hints));
									hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
									hints.ai_socktype = SOCK_STREAM; // Datagram socket
									hints.ai_flags = 0;
									hints.ai_protocol = 0;          // Any protocol
									/// hints specifies what the parameters for the search are, &result points to a list of addrinfo's
									if (getaddrinfo(request.host_name.c_str(), request.host_port.c_str(), &hints, &results) != 0) {
										perror("getaddrinfo() failed");
									}
									// go through all possible
									for (rp = results; rp != NULL; rp = rp->ai_next) {
										http_serv_sd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
										if (http_serv_sd == -1)
											continue;
										if (connect(http_serv_sd, rp->ai_addr, rp->ai_addrlen) != -1)
											break; // success
										close(http_serv_sd);
									}
									if (rp == NULL) { // no address succeeded
										std::cout << "Could not connect to host name and port" << std::endl;
										continue;
									}
									//freeaddrinfo(results);
									shared_->cached_addrinfo_map[request.host_name + ":" + request.host_port] = rp;
								}
								//if (write(http_serv_sd, buffer, bytes_received) != bytes_received) {
								if(SendWholeMessage(http_serv_sd, buffer, bytes_received) != bytes_received) {
										fprintf(stderr, "partial/failed write\n");
										exit(EXIT_FAILURE);
								}
								bzero(buffer, WEBPROXY_BUFFER_SIZE);
								bytes_received = read(http_serv_sd, buffer, WEBPROXY_BUFFER_SIZE);
								if (bytes_received == -1) {
									perror("read");
								}
								close(http_serv_sd);
								shared_->cached_page_map[request.request_line] = new CachedPage();
								cached_point = shared_->cached_page_map[request.request_line];
								pthread_mutex_lock(&cached_point->file_mx); // MUTEX lock file_mx
								std::cout << "<< caching page for \"" << request.request_line << "\"" << std::endl;
								time(&cached_point->created);
								strcat (cached_point->filename,"cache/");
								strcat (cached_point->filename, std::to_string(shared_->num_cached_pages++).c_str());
								strcat (cached_point->filename,".bin");
								cached_point->file = fopen(cached_point->filename, "wb");
								if (cached_point->file != NULL) {
									fwrite(buffer, bytes_received, 1, cached_point->file);
									//fputs(request.request_line.c_str(),cached_point->file);
									fclose(cached_point->file);
								}
								cached_point->file_size = (long) bytes_received;
								pthread_mutex_unlock(&cached_point->file_mx); // MUTEX unlock file_mx
								//write(i, buffer, bytes_received); // after caching page, send to client
								SendWholeMessage(i, buffer, bytes_received); // after caching page, send to client
							}
							else { // PAGE CACHED,
								pthread_mutex_lock(&cached_iter->second->file_mx); // MUTEX lock file_mx
								std::cout << ">> using cached page for \"" << request.request_line << "\"" << std::endl;
								bzero(buffer, WEBPROXY_BUFFER_SIZE);
								cached_iter->second->file = fopen(cached_iter->second->filename, "rb");
								fread (buffer,1,cached_iter->second->file_size,cached_iter->second->file);
								fclose(cached_iter->second->file);
								SendWholeMessage(i, buffer, cached_iter->second->file_size);
								pthread_mutex_unlock(&cached_iter->second->file_mx); //== UNLOCK
							}
							//*/
						}
					} // Enf of if i != listen_sd_
				} // End of if (FD_ISSET(i, &read_fds_))
			} // End of loop through selectable descriptors
		} // End of Else (socket is not listen_sd
	}
	// clean up open sockets
	for (i=0; i <= max_sd_; ++i) {
		if (FD_ISSET(i, &master_set_)) {
			close(i);
			if (i == listen_sd_) {
				std::cout << "- closing LISTENING socket (" << i << ")" << std::endl;
			}
			else {
				std::cout << "- closing client socket (" << i << ")" << std::endl;
			}
		}
	}
	free(buffer);
}
