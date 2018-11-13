#pragma once

#include <cctype>
#include <iostream>
#include <sstream>
#include <mutex>
#include <thread>
#include <iomanip>

/*
 * Simple logging macros, replace later with boost when we have a version with logging...
 */

//#include <boost/log/trivial.hpp>

enum LOG_LEVEL {
    TRACE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

// Make a better function name but not so complicated 
inline std::string simplify_pretty(const char *fname)
{
    std::string::size_type pos = 0;
    std::string sfname(fname);

    // Get the function name, i.e. '...foo(' without '('
    while (true) {
        pos = sfname.find('(', pos);
        if (pos == std::string::npos) {
          break;
        } else {
          if (pos > 0) {
              // CHeck if the previous character is graphics (it may be the end of the function)
              if (isgraph( sfname[pos-1] )) {
                  // We have the function name
                  break;
              }
          } 
          pos++;
        } 
    };

    if (pos == std::string::npos) {
      // Failed to obtain the name, return the full one
      return sfname;
    }

    // Get the function name
    std::string lfname = sfname.substr(0, pos);

    // Clean the function name
    pos = lfname.find_last_of(' ');

    if (pos != std::string::npos) {
      lfname = lfname.substr(pos+1);
    }
    
    return lfname;
}

//#define LOG(severity)   ( tools::log::debug() << "[0x" << std::hex << std::this_thread::get_id() << std::dec << "] " << severity << " [" TOOLS_DEBUG_INFO ", " << __PRETTY_FUNCTION__ << "]: " ) 
//#define LOG(severity)   ( tools::log::log() << "[0x" << std::hex << std::this_thread::get_id() << std::dec << "] " << severity << __func__ << ": " ) 
#define LOG(severity)   ( tools::log::log() << "[0x" << std::hex << std::this_thread::get_id() << std::dec << "] " << severity << simplify_pretty(__PRETTY_FUNCTION__) << ": " ) 


namespace tools {
namespace log {

// inline before moved somewhere else
inline std::ostream& operator<< (std::ostream& os, enum LOG_LEVEL severity)
{
    switch (severity)
    {
        case TRACE:     return os << "TRACE   " ;
        case DEBUG:     return os << "DEBUG   ";
        case INFO:      return os << "INFO    ";
        case WARNING:   return os << "WARNING ";
        case ERROR:     return os << "ERROR   ";
        case FATAL:     return os << "FATAL   ";
    };
    return os;
}


// From: https://stackoverflow.com/questions/2179623/how-does-qdebug-stuff-add-a-newline-automatically/2179782#2179782

static std::mutex lock;

struct log {
    log() {
    }

    ~log() {
        std::lock_guard<std::mutex> guard(lock);
        std::cout << os.str() << std::endl;
    }

public:
    // accepts just about anything
    template<class T>
    log& operator<<(const T& x) {
        os << x;
        return *this;
    }
private:
    std::ostringstream os;
};


} // namespace log
} // namespace tools
