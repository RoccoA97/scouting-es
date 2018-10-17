#include <iostream>
#include <fstream>
#include "config.h"

config::config(std::string filename){
  std::ifstream in(filename.c_str(),std::ios_base::in);
  std::string line;
  while ( !std::getline(in, line).eof() ) {

    // Perform left trim
    line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](int ch) {
        return !std::isspace(ch);
    }));

    // Skip empty lines
    if (line.empty()) {
      continue;
    }

    // Skip comments
    if (line.size() > 1 && line[0] == '#') {
      continue;
    }

    auto delim = line.find_first_of(":");
    if (delim == std::string::npos) {
      // Skip ill formated lines
       continue;
    }

    std::string key = line.substr(0, delim);
    std::string value = line.substr(delim + 1);

    if (key.empty() || value.empty()) {
      // Skip ill formated lines
       continue;      
    }

    //std::cout << "key: " << key << ", value: " << value << "\n";

    vmap[key] = value;
  }
}
void config::print()const {
  for (std::map<std::string,std::string>::const_iterator it = vmap.begin(); it!=vmap.end(); it++){
    std::cout << "key " << it->first << " value " << it->second << std::endl;
  }

}
