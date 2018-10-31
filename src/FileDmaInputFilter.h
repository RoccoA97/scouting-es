#ifndef FILE_DMA_INPUT_FILTER_H
#define FILE_DMA_INPUT_FILTER_H

#include <memory>
#include <string>
#include "tbb/pipeline.h"
#include "tbb/tick_count.h"

#include "InputFilter.h"

class FileDmaInputFilter: public InputFilter {
public:
  //FileDmaInputFilter( const std::string&, size_t, size_t);
  FileDmaInputFilter( const std::string& fileName, size_t packetBufferSize, size_t nbPacketBuffers, ctrl& control );
  virtual ~FileDmaInputFilter();

protected:
  ssize_t readInput(char **buffer, size_t bufferSize); // Override
  void print(std::ostream& out) const;  // Override

private:
  ssize_t readPacket(char **buffer, size_t bufferSize);

private:
  FILE* inputFile;

  struct Statistics {
    uint64_t nbOversizedPackets = 0;
  } stats;  
};

typedef std::shared_ptr<FileDmaInputFilter> FileDmaInputFilterPtr;

#endif // FILE_DMA_INPUT_FILTER_H
