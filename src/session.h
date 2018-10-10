#ifndef SESSION_H
#define SESSION_H
#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/tokenizer.hpp>
#include <string>
#include "controls.h"

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
  void handle_read(const boost::system::error_code& error,
      size_t bytes_transferred)
  {
    if (!error)
    {
      printf("handle read\n",data_);
      std::string s=data_;

      boost::tokenizer<> tok(s);
      boost::tokenizer<>::iterator i = tok.begin();
      std::string command = *i;
      std::string runno  = *(++i);
      if(command=="start" && !control->running){
	control->run_number = atoi(runno.c_str());
	control->running = true;
      }
      else if(command=="stop" && control->running){
	control->running = false;
      }
      sprintf(data_,"ok   ");
      bytes_transferred = reply_size;
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
      printf("handle write\n",data_);
      socket_.async_read_some(boost::asio::buffer(data_, max_length),
          boost::bind(&session::handle_read, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
      printf("what got %s\n",data_);
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
  static const size_t reply_size = 5;
};

#endif
