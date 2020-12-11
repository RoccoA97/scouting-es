#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "tbb/pipeline.h"
    
//reformatter

class Slice;

class StreamProcessor: public tbb::filter {
public:
  StreamProcessor(size_t, bool);
  void* operator()( void* item )/*override*/;
  ~StreamProcessor();

private:
  Slice* process(Slice& input, Slice& out);

private:
  size_t max_size;
  uint64_t nbPackets;
  bool doZS;
};

#endif
