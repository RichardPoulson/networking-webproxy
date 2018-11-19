#include "webproxy.h"

WebProxy::WebProxy(int port_num, int server_timeout)
{
}


WebProxy::~WebProxy()
{
}

const int WebProxy::get_max_num_threads() { return WebProxy::kMaxNumThreads; }

// Listen for HTTP requests from network hosts
bool WebProxy::ListenHTTPClients()
{
	// TODO: Add your implementation code here.
	return false;
}


// Accept HTTP connection that was received
int WebProxy::AcceptHTTPConnection()
{
	// TODO: Add your implementation code here.
	return 0;
}


int WebProxy::ParseRequest()
{
	// TODO: Add your implementation code here.
	return 0;
}


// Forward the HTTP request to the HTTP web server.
int WebProxy::ForwardRequest()
{
	// TODO: Add your implementation code here.
	return 0;
}


// Relay the data from the HTTP web server to the client.
int WebProxy::RelayData()
{
	// TODO: Add your implementation code here.
	return 0;
}


// Create a socket used to receive HTTP requests.
bool WebProxy::CreateSocket()
{
	// TODO: Add your implementation code here.
	return false;
}


// Bind the socket that will be used to receive HTTP requests.
bool WebProxy::BindSocket()
{
	// TODO: Add your implementation code here.
	return false;
}
