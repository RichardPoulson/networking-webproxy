#include "webserver.h"
#include "webproxy.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    cout << argc << endl;
    cout <<
      "Invalid number of additional arguments, please enter one number" <<
      "as argument." << endl;
      exit(EXIT_FAILURE);
  }
  WebProxy myWebProxy(atoi(argv[1]), atoi(argv[2]));
  WebServer myServer(8007); // first argument is string rep. of num.
}
