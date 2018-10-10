#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "tbb/pipeline.h"
    
//reformatter

class StreamProcessor: public tbb::filter {
public:
  StreamProcessor(size_t);
  void* operator()( void* item )/*override*/;
  ~StreamProcessor();
private:
  size_t max_size;
};

#endif
