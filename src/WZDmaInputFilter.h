#ifndef WZDMA_INPUT_FILTER_H
#define WZDMA_INPUT_FILTER_H

#include <iostream>
#include <memory>

#include "tbb/pipeline.h"
#include "tbb/tick_count.h"

#include "InputFilter.h"
#include "wz_dma.h"


class WZDmaInputFilter: public InputFilter {
 public:
  WZDmaInputFilter( size_t packetBufferSize, size_t nbPacketBuffers, ctrl& control );
  virtual ~WZDmaInputFilter();

protected:
  ssize_t readInput(char **buffer, size_t bufferSize); // Override
  void readComplete(char *buffer);  // Override
  void print(std::ostream& out) const;  // Override

private:
  ssize_t read_packet_from_dma(char **buffer);
  ssize_t read_packet( char **buffer, size_t bufferSize );

  struct Statistics {
    uint64_t nbDmaErrors = 0;
    uint64_t nbDmaOversizedPackets = 0;
    uint64_t nbBoardResets = 0;
  } stats;

  struct wz_private dma_;
};

typedef std::shared_ptr<WZDmaInputFilter> WZDmaInputFilterPtr;


#endif // WZDMA_INPUT_FILTER_H
