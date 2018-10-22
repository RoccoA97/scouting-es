#ifndef INPUT_FILTER_H
#define INPUT_FILTER_H

#include <cstddef>

#include "tbb/pipeline.h"
#include "tbb/tick_count.h"

#include "controls.h"

class Slice;

/*
 * This is an abstract class.
 * A dervide class has to implement methods readInput and readComplete
 */ 
class InputFilter: public tbb::filter {
public:
  //InputFilter( FILE*, size_t, size_t);
  InputFilter(size_t packet_buffer_size, size_t number_of_packet_buffers, ctrl& control);
  virtual ~InputFilter();

  uint64_t nbReads() { return nbReads_; }

protected:
  // Read input to a provided buffer or return a different buffer
  virtual ssize_t readInput(char **buffer, size_t bufferSize) = 0;

  // Notify the read that the buffer returned by readInput is processed (can be freed or reused)
  virtual void readComplete(char *buffer) = 0;

private:
  void* operator()(void* item); // Override

  inline ssize_t readHelper(char **buffer, size_t buffer_size);

private:
  ctrl& control_;
  //FILE* input_file;
  Slice* nextSlice_;
  //uint32_t counts;

  // Number of successfull reads
  uint64_t nbReads_;

  // Number of byted read
  uint64_t nbBytesRead_;

  // Number of read errors detected
  uint64_t nbErrors_;

  // Number of oversized packets returned ()
  uint64_t nbOversized_;  

  // Performance monitoring

  // Snapshot of nbBytesRead_
  uint64_t previousNbBytesRead_;

  // Remember timestamp for performance monitoring 
  tbb::tick_count previousStartTime_;
};

#endif // INPUT_FILTER_H 
