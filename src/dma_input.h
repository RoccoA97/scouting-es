#ifndef DMA_INPUT_H
#define DMA_INPUT_H

#include <memory>
#include <string>
#include "tbb/pipeline.h"
#include "tbb/tick_count.h"

class Slice;

class DmaInputFilter: public tbb::filter {
 public:
  DmaInputFilter( const std::string&, size_t, size_t);
  ~DmaInputFilter();
 private:
  int dma_fd;
  Slice* next_slice;
  void* operator()(void*) /*override*/;
  uint64_t counts;
  uint64_t ncalls;
  tbb::tick_count lastStartTime;
  uint64_t last_count;
};

typedef std::shared_ptr<DmaInputFilter> DmaInputFilterPtr;

#endif
