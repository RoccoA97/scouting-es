#include <iostream>
#include <fstream>
#include "config.h"

config::config(std::string filename){
  std::ifstream in(filename.c_str(),std::ios_base::in);
  std::string item;
  while(!std::getline(in,item,':').eof()){
    std::string value;
    std::getline(in,value);
    vmap[item]=value;
  }
}
void config::print()const {
  for(std::map<std::string,std::string>::const_iterator it = vmap.begin(); it!=vmap.end(); it++){
    std::cout << "key " << it->first << " value " << it->second << std::endl;
  }

}
