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
	// cast shared resources so we can use
	struct SharedResources * shared = (struct SharedResources*)shared_resources;
	char * buffer = (char*) malloc (WEBPROXY_BUFFER_SIZE);
	struct addrinfo hints, *res;
	int server_sd;
	int bytes_received, bytes_sent;
	std::string request, method, host, port;

	bzero(buffer, WEBPROXY_BUFFER_SIZE);
	pthread_mutex_lock(&shared->queue_mx); //== MUTEX
	int client_sd = shared->client_queue.front();
	shared->client_queue.pop();
	pthread_mutex_unlock(&shared->queue_mx); //== pthread_mutex_unlock
	if ((bytes_received = recv(client_sd, buffer, WEBPROXY_BUFFER_SIZE, 0)) < 0) {
		if (errno != EWOULDBLOCK) { // ERROR
			perror("recv() failed");
			free(buffer);
			close(client_sd);
			pthread_exit(NULL);
		}
	}
	else if (bytes_received == 0) { // Client closed connection
		free(buffer);
		close(client_sd);
		pthread_exit(NULL);
	}
	else { // Bytes were received from client
		std::cmatch match1, match2, match3; // first match with a line, then with what you're looking for
		std::regex_search (buffer, match1, shared->regex_map["host line"]);
		std::regex_search (match1.str().c_str(), match2, shared->regex_map["host name"]);
		host = match2.str();
		// Had to make custom regex to separate port number from address
		std::regex_search (match1.str().c_str(), match2, std::regex("(?![Host: " + host + ":])[^:]*"));
		port = match2.str();
		if (port == "") { port = "80"; } // if not defined, assume port 80
		std::regex_search (buffer, match1, shared->regex_map["request line"]);
		request = match1.str();
		std::regex_search (match1.str().c_str(), match2, shared->regex_map["method"]);
		method = match2.str();

		if (method != "GET") {
			char bad_request[] = "HTTP/1.1 400 Bad Request\r\n\r\n\r\n";
			if (SendWholeMessage(client_sd, bad_request, sizeof bad_request) < 0) {
				perror("send() failed");
			}
			close(client_sd);
			free(buffer);
			pthread_exit(NULL);
		}
		pthread_mutex_lock(&shared->cout_mx); //== MUTEX
		std::cout << "Request:  " << request << std::endl;
		pthread_mutex_unlock(&shared->cout_mx); //== pthread_mutex_unlock

		bzero(&hints, sizeof hints);
		hints.ai_family = AF_INET;
		if (getaddrinfo(host.c_str(), port.c_str(), &hints, &res) < 0) {
			perror("getaddrinfo() failed");
			free(buffer);
			close(client_sd);
			pthread_exit(NULL);
		}
		if ((server_sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
			perror("socket() failed");
			free(buffer);
			close(client_sd);
			pthread_exit(NULL);
		}
		if (connect(server_sd, res->ai_addr, res->ai_addrlen) < 0) {
			perror("connect() failed");
			free(buffer);
			close(client_sd);
			close(server_sd);
			pthread_exit(NULL);
		}
		if ((bytes_sent = SendWholeMessage(server_sd, buffer, bytes_received)) < 0) {
			perror("send() failed");
			close(client_sd);
			close(server_sd);
			pthread_exit(NULL);
		}
		pthread_mutex_lock(&shared->cout_mx); //== MUTEX
		std::cout << "Forward:  (" << request << ") to " << host << ":" << port << std::endl;
		pthread_mutex_unlock(&shared->cout_mx); //== UNLOCK
		// Clear buffer before receiving
		bzero(buffer, WEBPROXY_BUFFER_SIZE);
		if ((bytes_received = recv(server_sd, buffer, WEBPROXY_BUFFER_SIZE, 0)) < 0) {
			perror("received() failed");
			free(buffer);
			close(client_sd);
			close(server_sd);
			pthread_exit(NULL);
		}
		close(server_sd);
		if ((bytes_sent = SendWholeMessage(client_sd, buffer, bytes_received)) < 0) {
			perror("send() failed");
			free(buffer);
			close(client_sd);
			pthread_exit(NULL);
		}
		close(client_sd);
		free(buffer);
		pthread_exit(NULL);
	}
}

bool ProcessHTTPRequest(char * buf, struct RequestMessage * req) {
	std::cmatch cmatch1, cmatch2; // first match with a line, then with what you're looking for
	struct addrinfo hints;

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
	bzero(&req->host_addrinfo, sizeof req->host_addrinfo);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(req->host_name.c_str(), req->host_port.c_str(),
			&req->host_addrinfo, &req->host_res) != 0) {
		perror("getaddrinfo() failed");
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
}
CachedPage::~CachedPage() {
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
WebProxy::WebProxy(char * port_num, int timeout, std::string http_addr) {
	this->port_number_ = atoi(port_num);
	this->http_port_str_ = port_num;
	this->http_address_str_ = http_addr;
	this->timeout_.tv_sec = timeout; // for listening socket (in seconds)
	this->timeout_.tv_usec = 0;
	this->shared_ = new struct SharedResources();
	this->on_ = 1; // used by setsockopt to tell it to reuse socket address
	this->off_ = 0; // used to turn off setsockopt flags
	// Initialize and set thread joinable
  pthread_attr_init(&pthread_attr);
  pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_JOINABLE);
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
	long file_pos = 0;
	std::map<std::string, struct CachedPage *>::iterator cached_iter;
	struct CachedPage * cached_point;
	struct RequestMessage request;
	int i; // iterator for loop in StartHTTPServices()
	int http_serv_sd;
	int num_sockets_ready; // keeps track of how many socket descriptors are readable
	ssize_t bytes_received;
	struct sockaddr_in client_address; // client addr
	struct addrinfo hints, *result, *rp;
	socklen_t addr_length;
	char * buffer = (char*) malloc (WEBPROXY_BUFFER_SIZE);
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
						std::cout << "LISTENING socket added (" << new_sd_ << ")" << std::endl;
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
							std::cout << "closing Client socket (" << i << ")" << std::endl;
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
								std::cout << "Client " << i << ": bad request: \"" <<
										request.request_line << "\"" << std::endl;
								continue;
							}
							std::cout << "(" << i << ") [" << request.host_name << ":" <<
									request.host_port << "]  " << request.request_line << std::endl;
							cached_iter = shared_->cached_page_map.find(request.request_line);
							// PAGE NOT CACHED
							if (cached_iter == shared_->cached_page_map.end()) {
								bzero(&hints, sizeof(hints));
								hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
								hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
								hints.ai_flags = 0;
								hints.ai_protocol = 0;          /* Any protocol */
								//std::cout << request.request << std::endl;
								if (getaddrinfo(request.host_name.c_str(), request.host_port.c_str(), &hints, &result) != 0) {
									perror("getaddrinfo() failed");
								}
								for (rp = result; rp != NULL; rp = rp->ai_next) {
									http_serv_sd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
									if (http_serv_sd == -1)
										continue;
									if (connect(http_serv_sd, rp->ai_addr, rp->ai_addrlen) != -1)
										break;                  /* Success */
									close(http_serv_sd);
								}
								if (rp == NULL) {               /* No address succeeded */
									std::cout << "Could not connect" << std::endl;
									continue;
									// exit(EXIT_FAILURE);
								}
								freeaddrinfo(result);           /* No longer needed */
								if (write(http_serv_sd, buffer, bytes_received) != bytes_received) {
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
								std::cout << "  caching \"" << request.request_line << "\"" << std::endl;
								time(&cached_point->created);
								strcat (cached_point->filename,"cache/");
								strcat (cached_point->filename, std::to_string(shared_->num_cached_pages++).c_str());
								strcat (cached_point->filename,".txt");
								cached_point->file = fopen(cached_point->filename, "wb");
								if (cached_point->file != NULL) {
									fwrite(buffer, bytes_received, 1, cached_point->file);
									//fputs(request.request_line.c_str(),cached_point->file);
									fclose(cached_point->file);
								}
								cached_point->file_size = (long) bytes_received;
								write(i, buffer, bytes_received);
							}
							else { // PAGE CACHED
								std::cout << "  already cached \"" << request.request_line << "\" " <<
								difftime(time(NULL), cached_iter->second->created) << " seconds ago" << std::endl;
								bzero(buffer, WEBPROXY_BUFFER_SIZE);
								cached_iter->second->file = fopen(cached_iter->second->filename, "rb");
								fread (buffer,1,cached_iter->second->file_size,cached_iter->second->file);
								fclose(cached_iter->second->file);
								write(i, buffer, cached_iter->second->file_size);
								//cached_iter->second->file = fopen(cached_iter->second->filename,"rb");
								//write(i, buffer, bytes_received);
								//fclose(cached_iter->second->file);
							}
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
				std::cout << "closing LISTENING socket (" << i << ")" << std::endl;
			}
			else {
				std::cout << "closing Client socket (" << i << ")" << std::endl;
			}
		}
	}
	free(buffer);
}
