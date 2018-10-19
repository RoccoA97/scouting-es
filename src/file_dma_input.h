#ifndef FILE_DMA_INPUT_H
#define FILE_DMA_INPUT_H

#include <memory>
#include <string>
#include "tbb/pipeline.h"
#include "tbb/tick_count.h"

class Slice;

class FileDmaInputFilter: public tbb::filter {
 public:
  FileDmaInputFilter( const std::string&, size_t, size_t);
  ~FileDmaInputFilter();
 private:
  FILE* input_file;
  Slice* next_slice;
  void* operator()(void*) /*override*/;
  uint64_t counts;
  uint64_t ncalls;
  tbb::tick_count lastStartTime;
  uint64_t last_count;
};

typedef std::shared_ptr<FileDmaInputFilter> FileDmaInputFilterPtr;

#endif
