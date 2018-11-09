#ifndef SESSION_H
#define SESSION_H
#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <string>
#include "controls.h"

#include "log.h"

/*
 * string_view is available in C++17, let's make it available now with a little trick.
 * Remove this later, when we have C++17 compiler
 */
#include <boost/utility/string_ref.hpp> 

namespace std {
using string_view = boost::string_ref;
}
/*****************************************************************************/

using boost::asio::ip::tcp;



class session
{
public:
  session(boost::asio::io_service& io_service, ctrl *c)
    : socket_(io_service),
      control(c)
  {
  }

  tcp::socket& socket()
  {
    return socket_;
  }

  void start()
  {
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        boost::bind(&session::handle_read, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }

private:
  int process_command(std::string_view input, char *output, size_t size)
  {
    try {
      std::vector<std::string> items;
      boost::split(items, input, boost::is_any_of( " ," ), boost::token_compress_on);

      if ( items.size() < 1 || items.size() > 2 )  {
        return snprintf(output, size, "ERROR: Wrong number of arguments (%ld).", items.size());
      }
      const std::string& command = items[0];

      if ( command == "start" ) {
        if ( items.size() != 2) {
          return snprintf(output, size, "ERROR: Wrong number of arguments (%ld), expecting 2.", items.size());
        }
        uint32_t run_number = std::stoul( items[1] );

        if ( !control->running ) {
          control->run_number = run_number;
          control->running = true;
          return snprintf(output, size, "ok");

        } else {
          return snprintf(output, size, "ignored");
        }

      } else if ( command == "stop") {
        
        if ( control->running ) {
          control->running = false;
          return snprintf(output, size, "ok");

        } else {
          return snprintf(output, size, "ignored");
        }

      } else {
        return snprintf(output, size, "unknown command");
      }
    }
    catch (...) {
      return snprintf(output, size, "ERROR: Cannot parse input.");
    }
  }

  void handle_read(const boost::system::error_code& error,
      size_t bytes_transferred)
  {
    if (!error)
    {
      std::string_view input(data_, bytes_transferred);
      LOG(INFO) << "Run control: Received: '" << input << '\'';

      bytes_transferred = process_command(input, data_, max_length);

      std::string_view output(data_, bytes_transferred);
      LOG(INFO) << "Run control: Sending:  '" << output << '\'';

      boost::asio::async_write(socket_,
          boost::asio::buffer(data_, bytes_transferred),
          boost::bind(&session::handle_write, this,
            boost::asio::placeholders::error));
      
    }
    else
    {
      delete this;
    }
  }

  void handle_write(const boost::system::error_code& error)
  {
    if (!error)
    {
      //printf("handle write\n");
      socket_.async_read_some(boost::asio::buffer(data_, max_length),
          boost::bind(&session::handle_read, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
      //printf("what got '%s'\n",data_);
    }
    else
    {
      delete this;
    }
  }

  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
  ctrl *control;
  static const std::string reply_success;
  static const std::string reply_failure;
};

#endif
