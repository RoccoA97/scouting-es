#ifndef TOOLS_H
#define TOOLS_H

/*
 * Some useful functions
 */

#include <cstring>
#include <iostream>

#include <sys/stat.h>
#include <limits.h>

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


/*
 * Various filesystem related utilities (will be removed once moved to C++17, or rewritten with boost)
 */
namespace filesystem {

/*
 * Create the target directory and any parent directories if necessary
 */
inline bool create_directories(std::string& path)
{
  char tmp[PATH_MAX];

  // Add terminating '/' and make a writtable copy;
  int len = snprintf(tmp, sizeof(tmp), "%s/", path.c_str());
  if (len > PATH_MAX) len = PATH_MAX;

  char *last_backslash = tmp;
  for (char *p = tmp; *p; p++) {
    if (*p == '/') {
      // Found a new directory, ignore any subsequent back slashes
      int dir_len = p - last_backslash - 1;
      if (dir_len > 0) {
        *p = 0;
        if (mkdir(tmp, S_IRWXU | S_IRWXG | S_IRWXO) < 0 && (errno != EEXIST)) {
          return false;
        }
        *p = '/';
      }
      last_backslash = p;
    }
  }

  return true;
}


} // namespace filesystem
} // namespace tools


#endif // TOOLS_H
