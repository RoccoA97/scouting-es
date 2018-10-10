#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <stdint.h>
#include "boost/lexical_cast.hpp"
#include <map>
#include <stdexcept>

class config{
public:
  

  config(std::string filename);

  void print() const;

  std::string getInputFile() const {
    std::string filename;
    try{
      filename = vmap.at("input_file");
    }catch(const std::out_of_range& oor){
      
    }
    return filename;
  }
  std::string getElasticUrl() const
  {
    return vmap.at("elastic_url");
  }
  uint32_t getQualCut() const {
    std::string v = vmap.at("quality_cut");
    return boost::lexical_cast<uint32_t>(v.c_str());
  }
  uint32_t getPtCut() const {
    std::string v = vmap.at("pt_cut");
    return boost::lexical_cast<uint32_t>(v.c_str());
  }
  std::string getOutputFilenameBase() const 
  {
    return vmap.at("output_filename_base");
  }
  uint64_t getOutputMaxFileSize() const {
    std::string v = vmap.at("max_file_size");
    return boost::lexical_cast<uint64_t>(v.c_str());
  }
  uint32_t getNumThreads() const {
    std::string v = vmap.at("threads");
    return boost::lexical_cast<uint32_t>(v.c_str());
  }
  uint32_t getNumInputBuffers() const {
    std::string v = vmap.at("input_buffers");
    return boost::lexical_cast<uint32_t>(v.c_str());
  }
  uint32_t getBlocksPerInputBuffer() const {
    std::string v = vmap.at("blocks_buffer");
    return boost::lexical_cast<uint32_t>(v.c_str());
  }
  short getPortNumber() const {
    std::string v = vmap.at("port");
    return boost::lexical_cast<short>(v.c_str());
  }
  
private:
  
  std::map<std::string,std::string> vmap;
  

};
#endif
