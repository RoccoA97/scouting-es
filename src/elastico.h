#ifndef ELASTICO_H
#define ELASTICO_H

#include <cstdint>
#include <string>
#include <sstream>
#include <iostream>
#include "curl/curl.h"
#include "tbb/pipeline.h"
    
class ctrl;

//reformatter

class ElasticProcessor: public tbb::filter {
public:
  ElasticProcessor(size_t, ctrl *, std::string, uint32_t, uint32_t);
  void* operator()( void* item )/*override*/;
  ~ElasticProcessor();
private:
  void makeCreateIndexRequest(unsigned int);
  void makeAppendToBulkRequest(std::ostringstream &, char *);
  size_t max_size;
  ctrl *control;
  std::string request_url; 
  std::string c_request_url;
  uint32_t pt_cut;
  uint32_t qual_cut;
  CURL *handle;
  struct curl_slist *headers;
};

#endif
