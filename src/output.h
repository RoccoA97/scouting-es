#ifndef OUTPUT_H
#define OUTPUT_H

#include <cstdio>
#include <stdint.h>
#include <string>
#include "tbb/pipeline.h"

#include "controls.h"

//! Filter that writes each buffer to a file.
class OutputStream: public tbb::filter {


public:
  OutputStream( const char*, ctrl *c );
  void* operator()( void* item ) /*override*/;

private:
  void open_next_file();
  std::string my_output_file_base;
  uint32_t totcounts;
  uint64_t current_file_size;
  uint32_t file_count;
  ctrl *control;
  FILE *current_file;
};

#endif
