#ifndef WZDMA_INPUT_H
#define WZDMA_INPUT_H

#include <memory>
#include <string>

#include "tbb/pipeline.h"
#include "tbb/tick_count.h"

#include "wz_dma.h"

class Slice;

class WZDmaInputFilter: public tbb::filter {
 public:
  WZDmaInputFilter(size_t, size_t);
  ~WZDmaInputFilter();

 private:
  ssize_t read_packet(char **buffer);
  struct wz_private dma;
  Slice* next_slice;
  void* operator()(void*) /*override*/;
  uint64_t counts;
  uint64_t ncalls;
  tbb::tick_count lastStartTime;
  uint64_t last_count;
  uint64_t dma_errors;
};

typedef std::shared_ptr<WZDmaInputFilter> WZDmaInputFilterPtr;

#endif
