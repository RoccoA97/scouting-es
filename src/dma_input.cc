#include <cassert>
#include <cstdio>
#include <cerrno>
#include <system_error>
#include <iostream>
#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dma_input.h"
#include "slice.h"


DmaInputFilter::DmaInputFilter( const std::string& dma_dev_, size_t packet_buffer_size_, 
			  size_t number_of_packet_buffers_) : 

  filter(serial_in_order),
  next_slice(Slice::preAllocate( packet_buffer_size_, number_of_packet_buffers_) ),
  counts(0),
  ncalls(0),
  lastStartTime(tbb::tick_count::now()),
  last_count(0)
{ 
  dma_fd = open( dma_dev_.c_str(), O_RDWR | O_NONBLOCK );
  if ( dma_fd < 0 ) {
    throw std::system_error(errno, std::system_category(), "Cannot open DMA device" + dma_dev_);
  }

  fprintf(stderr,"Created input dma filter and allocated at 0x%llx \n",(unsigned long long)next_slice);
}

DmaInputFilter::~DmaInputFilter() {
  fprintf(stderr,"Destroy input dma filter and delete at 0x%llx \n",(unsigned long long)next_slice);
  Slice::giveAllocated(next_slice);
  fprintf(stderr,"input operator total %lu  read \n",counts);
  close( dma_fd );
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

void* DmaInputFilter::operator()(void*) {
  size_t buffer_size = next_slice->avail();
  ssize_t bytes_read = 0;

  // We need at least 1MB buffer
  assert( buffer_size >= 1024*1024 );

  while (true) {
    // Count reads
     ncalls++;

    // Read from DMA
    bytes_read = read_axi_packet_to_buffer(dma_fd, next_slice->begin(), buffer_size);

    if (bytes_read < 0) {
      // Check for errors we can skip
      if (errno == EIO || errno == EMSGSIZE) {
        std::cerr << "Iteration: " << ncalls; 
        if (errno == EIO) {
          std::cerr << " DMA ERROR: I/O ERROR, skipping packet...\n"; 
        } else {
          std::cerr << " DMA ERROR: Packet too long, skipping packet...\n"; 
        }
        continue;
      }

      // Some fatal error occured
      std::ostringstream os;
      os  << "Iteration: " << ncalls 
          << "  ERROR: DMA read failed.";
      throw std::system_error(errno, std::system_category(), os.str() );
    }

    // We have some data
    break;
  }

  // This should not happen
  if (bytes_read > (ssize_t)buffer_size ){
    std::ostringstream os;
    os  << "Iteration: " << ncalls 
        << "  ERROR: DMA read returned " << bytes_read 
        << " > buffer size " << buffer_size;
    throw std::runtime_error( os.str() );
  }

  // This should really not happen
  assert( bytes_read != 0);

  // Calculate DMA bandwidth
  tbb::tick_count now = tbb::tick_count::now();
  double time_diff =  (double)((now - lastStartTime).seconds());
  lastStartTime = now;
  double bwd = bytes_read / ( time_diff * 1024.0 * 1024.0 );

  std::cout << "Read returned: " << bytes_read << ", DMA bandwidth " << bwd << "MBytes/sec\n";

  // Have more data to process.
  Slice* this_slice = next_slice;
  next_slice = Slice::getAllocated();
  
  // Adjust the end of this buffer
  this_slice->set_end( this_slice->end() + bytes_read );
  
  return this_slice;

}
