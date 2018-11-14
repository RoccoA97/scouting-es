#ifndef TOOLS_H
#define TOOLS_H

/*
 * Some useful functions
 */

#include <cstring>
#include <iostream>

namespace tools {


/*
 * A thread-safe version of strerror: Returns a C++ std::string describing error_code
 */                     
inline const std::string strerror(int error_code) 
{               
  char local_buf[128];                                      
  char *buf = local_buf;                                    
  // NOTE: The returned buf may not be the same as the original buf
  buf = strerror_r(error_code, buf, sizeof(local_buf));     
  // Make a proper C++ string out of the returned buffer
  std::string str(buf);                                     
  return str;                                               
}    

/*
 * A thread-safe version of strerror: Returns a C++ std::string describing ERRNO
 */ 
inline const std::string strerror() 
{
  return tools::strerror(errno);
}

/*
 * A thread-safe version of "perror". Prepends a custom message before the error code and returns everything in std::string
 */ 
inline const std::string strerror(const std::string& msg)
{
  return msg + ": " + tools::strerror();
}


} // namespace tools


#endif // TOOLS_H
