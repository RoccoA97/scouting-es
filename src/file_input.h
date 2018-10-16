#ifndef INPUT_H
#define INPUT_H

#include <string>
#include "tbb/pipeline.h"
#include "tbb/tick_count.h"

class Slice;

class FileInputFilter: public tbb::filter {
 public:
  FileInputFilter( const std::string&, size_t, size_t);
  ~FileInputFilter();
 private:
  FILE* input_file;
  Slice* next_slice;
  void* operator()(void*) /*override*/;
  uint64_t counts;
  uint64_t ncalls;
  tbb::tick_count lastStartTime;
  uint64_t last_count;
};

#endif
