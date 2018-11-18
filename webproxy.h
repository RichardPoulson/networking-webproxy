/*
 * @author Richard Poulson
 * @date 6 Oct 2018
 *
 * WebProxy object:
 * - receives HTTP requests, parses and verifies the data.
 *
 * @references:
 *   https://www.geeksforgeeks.org/socket-programming-cc/
 *   https://stackoverflow.com/questions/1151582/pthread-function-from-a-class
 *   https://stackoverflow.com/questions/4250013/is-destructor-called-if-sigint-or-sigstp-issued
 *   http://www.cplusplus.com/reference/fstream/basic_ifstream/rdbuf/
 */

// if buffer is larger than ~ 200KB, may have to put buffers in heap (dynamic)

#ifndef NETWORKING_WEBPROXY_H_
#define NETWORKING_WEBPROXY_H_

//== WebProxy, object contains parent sockets, pthreads create client socket
class WebProxy
{
public:
  WebProxy(int port_number, int max_mumbers_threads);
  ~WebProxy();
protected:
private:
  bool proceed;
};
#endif  // NETWORKING_WEBPROXY_H_
