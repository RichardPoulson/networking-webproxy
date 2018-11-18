#ifndef NETWORKING_WEBPROXY_H_
#define NETWORKING_WEBPROXY_H_

class WebProxy
{
public:
	WebProxy();
	virtual ~WebProxy();
	const static int get_max_num_threads();
private:
	bool http_service_enabled;
	// Maximum number of threads used to service HTTP clients.
	static const int kMaxNumThreads = 8;
	static const int kBufferSize = 16777216; // 16MB
protected:
	// Listen for HTTP requests from network hosts
	bool ListenHTTPClients();
	// Accept HTTP connection that was received
	int AcceptHTTPConnection();
	int ParseRequest();
	// Forward the HTTP request to the HTTP web server.
	int ForwardRequest();
	// Relay the data from the HTTP web server to the client.
	int RelayData();
};
#endif // NETWORKING_WEBPROXY_H_
