#ifndef SERVER_H
#define SERVER_H
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "session.h"
#include "controls.h"

class server
{
public:
  server(boost::asio::io_service& io_service, short port, ctrl& c)
    : io_service_(io_service),
      acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
      control(c)
  {
    start_accept(control);
  }

private:
  void start_accept(ctrl& c)
  {
    session* new_session = new session(io_service_,c);
    acceptor_.async_accept(new_session->socket(),
        boost::bind(&server::handle_accept, this, new_session,
          boost::asio::placeholders::error));
  }

  void handle_accept(session* new_session,
      const boost::system::error_code& error)
  {
    if (!error)
    {
      new_session->start();
    }
    else
    {
      delete new_session;
    }

    start_accept(control);
  }

  boost::asio::io_service& io_service_;
  tcp::acceptor acceptor_;
  ctrl& control;
};


#endif 
