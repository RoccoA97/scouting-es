#include <cassert>
#include <cstdio>
#include <cerrno>
#include <system_error>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "wzdma_input.h"
#include "slice.h"


WZDmaInputFilter::WZDmaInputFilter(size_t packet_buffer_size_, size_t number_of_packet_buffers_, ctrl* control_) : 

  filter(serial_in_order),
  next_slice(Slice::preAllocate( packet_buffer_size_, number_of_packet_buffers_) ),
  counts(0),
  ncalls(0),
  lastStartTime(tbb::tick_count::now()),
  last_count(0),
  dma_errors(0),
  dma_oversized(0),
  control(control_)
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
const std::string strerror(int error_code) 
{               
  // TODO: Prealocat std::String and use directly its buffer
  char local_buf[128];                                      
  char *buf = local_buf;                                    
  buf = strerror_r(error_code, buf, sizeof(local_buf));     
  std::string str(buf);                                     
  return str;                                               
}    

const std::string strerror() 
{
  return tools::strerror(errno);
}

const std::string strerror(const std::string& msg)
{
  return msg + ": " + tools::strerror();
}

void perror(const std::string& msg)
{
  std::cerr << tools::strerror(msg) << std::endl;
}
} // namespace tools


inline ssize_t WZDmaInputFilter::read_packet(char **buffer)
{
  int tries = 1;
  ssize_t bytes_read;

  while (1) {
    bytes_read = wz_read_start( &dma, buffer );

    if (bytes_read < 0) {
      dma_errors++;
      tools::perror("#" + std::to_string(ncalls) + ": Read failed");

      if (errno == EIO) {
        std::cerr << "#" << ncalls << ": Trying to restart DMA (attempt #" << tries << "): ";
        wz_stop_dma( &dma );
        wz_close( &dma );

        // Initialize the DMA subsystem 
        if ( wz_init( &dma ) < 0 ) {
          throw std::system_error(errno, std::system_category(), "Cannot initialize WZ DMA device");
        }

        if (wz_start_dma( &dma ) < 0) {
          throw std::system_error(errno, std::system_category(), "Cannot start WZ DMA device");
        }

        std::cerr << "Success.\n";
        tries++;

        if (tries == 10) {
          throw std::runtime_error("FATAL: Did not manage to restart DMA.");
        }
        continue;
      }
      throw std::system_error(errno, std::system_category(), "FATAL: Unrecoverable DMA error detected.");
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
  int skip = 0;
  bytes_read = read_packet( &buffer );

  // If large packet returned, skip and read again
  while ( bytes_read > (ssize_t)buffer_size ) {
    dma_oversized++;
    skip++;
    std::cerr 
      << "#" << ncalls << ": ERROR: DMA read returned " << bytes_read << " > buffer size " << buffer_size
      << ". Skipping packet #" << skip << ".\n";
    if (skip >= 100) {
      throw std::runtime_error("FATAL: DMA is still returning large packets.");
    }
    bytes_read = read_packet( &buffer );
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
  if (control->packets_per_report && (ncalls % control->packets_per_report == 0)) {
    // Calculate DMA bandwidth
    tbb::tick_count now = tbb::tick_count::now();
    double time_diff =  (double)((now - lastStartTime).seconds());
    lastStartTime = now;
    double bwd = counts / ( time_diff * 1024.0 * 1024.0 );

    std::cout 
      << "#" << ncalls << ": Read(s) returned: " << counts 
      << ", DMA bandwidth " << bwd << "MBytes/sec, DMA errors " << dma_errors 
      << ", DMA oversized packets " << dma_oversized << ".\n";
    counts = 0;
  }

  // Have more data to process.
  Slice* this_slice = next_slice;
  next_slice = Slice::getAllocated();
  
  // Adjust the end of this buffer
  this_slice->set_end( this_slice->end() + bytes_read );
  
  return this_slice;

}
