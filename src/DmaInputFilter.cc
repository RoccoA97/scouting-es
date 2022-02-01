#include <cassert>
#include <cstdio>
#include <cerrno>
#include <system_error>
#include <iostream>
#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "DmaInputFilter.h"
#include "log.h"



DmaInputFilter::DmaInputFilter( const std::string& deviceFileName, size_t packetBufferSize, size_t nbPacketBuffers, ctrl& control ) :
  InputFilter( packetBufferSize, nbPacketBuffers, control )
{
  dma_fd = open( deviceFileName.c_str(), O_RDWR | O_NONBLOCK );
  if ( dma_fd < 0 ) {
    throw std::system_error(errno, std::system_category(), "Cannot open DMA device: " + deviceFileName);
  }

  LOG(TRACE) << "Created DMA input filter";
}

DmaInputFilter::~DmaInputFilter() {
  close( dma_fd );
  LOG(TRACE) << "Destroyed DMA input filter";
}

/*
 * man 2 write:
 * On Linux, write() (and similar system calls) will transfer at most
 * 	0x7ffff000 (2,147,479,552) bytes, returning the number of bytes
 *	actually transferred.  (This is true on both 32-bit and 64-bit
 *	systems.)
 */

#define RW_MAX_SIZE	0x7ffff000

static inline ssize_t read_axi_packet_to_buffer(int fd, char *buffer, uint64_t size)
{
  ssize_t rc;
  uint64_t to_read = size;

  if (to_read > RW_MAX_SIZE) {
    to_read = RW_MAX_SIZE;
  }

  /* read data from file into memory buffer */
  rc = read(fd, buffer, to_read);

  if (rc <= 0) {
    return rc;
  }
  return rc;
}

void print256(void *buf, int count)
{
  uint32_t	*u32p = (uint32_t*) buf;

  for (int i=0; i < count; ++i){
    printf("0x%08x_%08x_%08x_%08x_%08x_%08x_%08x_%08x\n", u32p[8*i+7], u32p[8*i+6], u32p[8*i+5],u32p[8*i+4], u32p[8*i+3], u32p[8*i+2], u32p[8*i+1], u32p[8*i]);
  }
}

// read_axi_packet_to_buffer for firmware with header
static inline ssize_t read_axi_packet_to_buffer_header(int fd, char *buffer, uint64_t nbReads)
{
  // ssize_t rc;
  ssize_t rc1, rc2, rc3;
  // uint64_t to_read = size;

  uint32_t *u32p;
  uint32_t packetSize;

  // read 32 bytes until header is found
  do{
    rc1 = read(fd, buffer, 32);
    if (rc1 < 0) {
      LOG(ERROR) << "#" << nbReads << ": DMA I/O ERROR. Failed reading header. Error = " << rc1;
      throw std::runtime_error( "read_axi_packet_to_buffer_header finished with error" );
    }
    u32p = (uint32_t*) buffer;

    // debug
    print256(buffer, 1)
  }
  while( *u32p != 4276993775 ); // feedbeef in decimal

  // packet length from header
  packetSize = 32*(*((uint32_t*) (buffer + 8)) + 10);
  std::cout << "packetSize " << packetSize << std::endl;

  if (packetSize > RW_MAX_SIZE) {
    LOG(ERROR) << "#" << nbReads << ": DMA I/O ERROR. Packet size exceeds maximum allowed.";
    throw std::runtime_error( "read_axi_packet_to_buffer_header finished with error" );
  }

  // read packet content
  rc2 = read(fd, buffer, packetSize);
  if (rc2 < 0) {
    LOG(ERROR) << "#" << nbReads << ": DMA I/O ERROR. Failed reading packet content. Error = " << rc2;
    throw std::runtime_error( "read_axi_packet_to_buffer_header finished with error" );
  }

  // debug
  print256(buffer, 1)

  // // read trailer
  // rc3 = read(fd, buffer, 32);
  // if (rc3 < 0) {
  //   LOG(ERROR) << "#" << nbReads << ": DMA I/O ERROR. Failed reading packet trailer. Error = " << rc3;
  //   throw std::runtime_error( "read_axi_packet_to_buffer_header finished with error" );
  // }

  return rc1+rc2;
}

ssize_t DmaInputFilter::readPacketFromDMA(char **buffer, size_t bufferSize)
{
  // Read from DMA

  ssize_t bytesRead = 0;
  int skip = 0;

  while (true) {
    // Read from DMA
    // bytesRead = read_axi_packet_to_buffer(dma_fd, *buffer, bufferSize);
    bytesRead = read_axi_packet_to_buffer_header(dma_fd, *buffer, nbReads());

    if (bytesRead < 0) {
      skip++;
      // Check for errors and then skip
      if (errno == EIO || errno == EMSGSIZE) {
        if (errno == EIO) {
          stats.nbDmaErrors++;
          LOG(ERROR) << "#" << nbReads() << ": DMA I/O ERROR. Skipping packet #" << skip << '.';
        } else {
          stats.nbDmaOversizedPackets++;
          LOG(ERROR) << "#" << nbReads() << ": DMA read returned oversized packet. DMA returned " << bytesRead << ", buffer size is " << bufferSize << ". Skipping packet #" << skip << '.';
        }
        continue;
      }

      // Some fatal error occurred
      std::ostringstream os;
      os  << "Iteration: " << nbReads() << "  ERROR: DMA read failed.";
      throw std::system_error(errno, std::system_category(), os.str() );
    }

    // We have some data
    break;
  }

  return bytesRead;
}


/**************************************************************************
 * Entry points are here
 * Overriding virtual functions
 */


// Print some additional info
void DmaInputFilter::print(std::ostream& out) const
{
    out
      << ", DMA errors " << stats.nbDmaErrors
      << ", oversized " << stats.nbDmaOversizedPackets;
}


// Read a packet from DMA
ssize_t DmaInputFilter::readInput(char **buffer, size_t bufferSize)
{
  return readPacketFromDMA( buffer, bufferSize );
}
