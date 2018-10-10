#ifndef INPUT_H
#define INPUT_H

#include "tbb/pipeline.h"

class Slice;

class InputFilter: public tbb::filter {
 public:
  InputFilter( FILE*, size_t, size_t);
  ~InputFilter();
 private:
  FILE* input_file;
  Slice* next_slice;
  void* operator()(void*) /*override*/;
  uint32_t counts;
};

#endif
