#ifndef CONTROLS_H
#define CONTROLS_H
#include <stdint.h>

struct ctrl{
  uint32_t run_number;
  bool running;
  uint64_t max_file_size;
};
#endif 
