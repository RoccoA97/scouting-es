#include <cassert>
#include <cstdio>
#include <cerrno>
#include <system_error>
#include <iostream>
#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "wzdma_input.h"
#include "slice.h"


WZDmaInputFilter::WZDmaInputFilter(size_t packet_buffer_size_, size_t number_of_packet_buffers_) : 

  filter(serial_in_order),
  next_slice(Slice::preAllocate( packet_buffer_size_, number_of_packet_buffers_) ),
  counts(0),
  ncalls(0),
  lastStartTime(tbb::tick_count::now()),
  last_count(0),
  dma_errors(0)
{ 
  // Initialize the DMA subsystem 
	if ( wz_init( &dma ) < 0 ) {
    throw std::system_error(errno, std::system_category(), "Cannot initialize WZ DMA device");
  }

	// Start the DMA
	if ( wz_start_dma( &dma ) < 0) {
    throw std::system_error(errno, std::system_category(), "Cannot start WZ DMA");
	}

  fprintf(stderr,"Created input wzdma filter and allocated at 0x%llx \n",(unsigned long long)next_slice);
}

WZDmaInputFilter::~WZDmaInputFilter() {
  fprintf(stderr,"Destroy input dma filter and delete at 0x%llx \n",(unsigned long long)next_slice);
  Slice::giveAllocated(next_slice);
  fprintf(stderr,"input operator total %lu  read \n", counts);

  wz_stop_dma( &dma );
	wz_close( &dma );
}


namespace tools {
// Returns a C++ std::string describing ERRNO                     
const std::string strerror(int error_code) {               
  // TODO: Prealocat std::String and use directly its buffer
  char local_buf[128];                                      
  char *buf = local_buf;                                    
  buf = strerror_r(error_code, buf, sizeof(local_buf));     
  std::string str(buf);                                     
  return str;                                               
}    
} // namespace tools


inline ssize_t WZDmaInputFilter::read_packet(char **buffer)
{
  int tries = 0;
  ssize_t bytes_read;

      // // Some fatal error occured
      // std::ostringstream os;
      // os  << "Iteration: " << ncalls 
      //     << "  ERROR: DMA read failed.";
      // throw std::system_error(errno, std::system_category(), os.str() );


  while (1) {
    bytes_read = wz_read_start( &dma, buffer );

    if (bytes_read < 0) {
      dma_errors++;
      perror("Read failed");

      if (errno == EIO) {
        fprintf(stderr, "Trying to restart DMA: ");
        wz_stop_dma( &dma );

        if (wz_start_dma( &dma ) < 0) {
          throw std::system_error(errno, std::system_category(), "Cannot start WZ DMA device");
        }

        fprintf(stderr, "Success.\n");
        tries++;

        if (tries == 10) {
          fprintf(stderr, "We are not advancing, terminating.\n");
          exit(5);
        }

        continue;
      }
      exit(4);
    }
    break;
  }

  // Should not happen
  assert(bytes_read > 0);  

  return bytes_read;
}


void* WZDmaInputFilter::operator()(void*) {
  size_t buffer_size = next_slice->avail();
  ssize_t bytes_read = 0;

  char *buffer = NULL;

  // We need at least 1MB buffer
  assert( buffer_size >= 1024*1024 );

  ncalls++;

  // Read from DMA
  //bytes_read = read_axi_packet_to_buffer(dma_fd, next_slice->begin(), buffer_size);
  bytes_read = read_packet( &buffer );

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

  // We copy data from the DMA buffer
  // NOTE: This is stupid, we should use zero copy approach 
  memcpy( next_slice->begin(), buffer, bytes_read );

  // Free DMA buffer
  if ( wz_read_complete( &dma ) < 0 ) {
      throw std::system_error(errno, std::system_category(), "Cannot complete WZ DMA read");
	}

  counts += bytes_read;

  // TODO: Make this configurable
  if (ncalls % 5000 == 0) {
    // Calculate DMA bandwidth
    tbb::tick_count now = tbb::tick_count::now();
    double time_diff =  (double)((now - lastStartTime).seconds());
    lastStartTime = now;
    double bwd = counts / ( time_diff * 1024.0 * 1024.0 );

    std::cout << "#" << ncalls << ": Read(s) returned: " << counts << ", DMA bandwidth " << bwd << "MBytes/sec\n";
    counts = 0;
  }

  // Have more data to process.
  Slice* this_slice = next_slice;
  next_slice = Slice::getAllocated();
  
  // Adjust the end of this buffer
  this_slice->set_end( this_slice->end() + bytes_read );
  
  return this_slice;

}
