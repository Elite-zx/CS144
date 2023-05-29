#include "address.hh"
#include "socket.hh"

#include <asm-generic/errno-base.h>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <sys/socket.h>

using namespace std;

void get_URL( const string& host, const string& path )
{
  // cerr << "Function called: get_URL(" << host << ", " << path << ")\n";
  // cerr << "Warning: get_URL() has not been implemented yet.\n";
  TCPSocket tcp_socket {};
  const Address target_addr( host, "http" );
  tcp_socket.connect( target_addr );
  string request { "GET " + path + " HTTP/1.1\r\n" }; // can not use '+' in string_view object
  cout << request;
  string header0 { "Host: " + host + "\r\n" };
  cout << header0;
  string_view header1 { "Connection: close\r\n" };
  cout << header1;
  string_view empty_line { "\r\n" };
  cout << empty_line;
  tcp_socket.write( request );
  tcp_socket.write( header0 );
  tcp_socket.write( header1 );
  tcp_socket.write( empty_line );
  string rcv_buffer;
  while ( !tcp_socket.eof() ) {
    tcp_socket.read( rcv_buffer );
    cout << rcv_buffer;
    rcv_buffer.clear();
  }
  tcp_socket.close();
  // destructor
}

int main( int argc, char* argv[] )
{
  try {
    if ( argc <= 0 ) {
      abort(); // For sticklers: don't try to access argv[0] if argc <= 0.
    }

    auto args = span( argv, argc );

    // The program takes two command-line arguments: the hostname and "path" part of the URL.
    // Print the usage message unless there are these two arguments (plus the program name
    // itself, so arg count = 3 in total).
    if ( argc != 3 ) {
      cerr << "Usage: " << args.front() << " HOST PATH\n";
      cerr << "\tExample: " << args.front() << " stanford.edu /class/cs144\n";
      return EXIT_FAILURE;
    }

    // Get the command-line arguments.
    const string host { args[1] };
    const string path { args[2] };

    // Call the student-written function.
    get_URL( host, path );
  } catch ( const exception& e ) {
    cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
