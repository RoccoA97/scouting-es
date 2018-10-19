#include <cassert>
#include <cstdio>
#include <cerrno>
#include <system_error>
#include <iostream>
#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "file_dma_input.h"
#include "slice.h"


FileDmaInputFilter::FileDmaInputFilter( const std::string& file_name_, size_t packet_buffer_size_, 
			  size_t number_of_packet_buffers_) : 

  filter(serial_in_order),
  next_slice(Slice::preAllocate( packet_buffer_size_, number_of_packet_buffers_) ),
  counts(0),
  ncalls(0),
  lastStartTime(tbb::tick_count::now()),
  last_count(0)
{ 
  input_file = fopen( file_name_.c_str(), "r" );
  if ( !input_file ) {
    throw std::invalid_argument( "Invalid input file name: " + file_name_ );
  }
  fprintf(stderr,"Created input dma filter and allocated at 0x%llx \n",(unsigned long long)next_slice);
}

FileDmaInputFilter::~FileDmaInputFilter() {
  fprintf(stderr,"Destroy input dma filter and delete at 0x%llx \n",(unsigned long long)next_slice);
  Slice::giveAllocated(next_slice);
  fprintf(stderr,"input operator total %lu  read \n",counts);
  //  fclose( output_file );
  fclose( input_file );
}

/*
 * This function reads packet by packet from a file
 * in order to simulate DMA reads.
 * 
 * TODO: Better would be mmap directly the file
 */ 
static inline ssize_t read_dma_packet_from_file(FILE *input_file, char *buffer, uint64_t size, uint64_t pkt_count)
{
  static constexpr uint64_t deadbeaf = 0xdeadbeefdeadbeefL;
  size_t bytes_read = 0;
  size_t rc;

  while ( !feof(input_file) && bytes_read < size) {
    // Expecting 32 byte alignment 
    rc = fread( buffer, 1, 32, input_file );

    if (ferror(input_file)) {
        throw std::system_error(errno, std::system_category(), "File error");
    }

    if (rc != 32) {
      if ( feof(input_file) && rc == 0 && bytes_read == 0) {
        // We have reached the perfect end of the file, let's start again
        fseek(input_file, 0, SEEK_SET);
        continue;
      }

      // Misaligned data
      throw std::runtime_error("#" + std::to_string(pkt_count) + ": File read ends prematurely, missing " + std::to_string(32-rc) + " bytes. Something is probably misaligned.");
    }

    bytes_read += 32;

    if ( *(uint64_t*)(buffer) == deadbeaf ) {
      // End of the packet was found
      return bytes_read;
    }

    buffer += 32;
  }

  // We are here either
  //   because we found EOF earlier than the end of the packet 
  //   or the packet cannot fit into the buffer
  // We can deal with both conditions but for the moment we throw an error 

  if (feof(input_file)) {
    throw std::runtime_error("EOF reached but no end of the packet found.");
  }

  throw std::runtime_error("Packet is too big and cannot fit into preallocated memory");
  // TODO: Read and discard data until deadbeef is found

  return -1;
}


void* FileDmaInputFilter::operator()(void*) {
  size_t buffer_size = next_slice->avail();
  ssize_t bytes_read = 0;

  // We need at least 1MB buffer
  assert( buffer_size >= 1024*1024 );

  while (true) {
    // Count reads
     ncalls++;

    // Read from DMA
    bytes_read = read_dma_packet_from_file(input_file, next_slice->begin(), buffer_size, ncalls);

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

  std::cout << "#" << ncalls << ": Read returned: " << bytes_read << ", DMA bandwidth " << bwd << "MBytes/sec\n";

  // Have more data to process.
  Slice* this_slice = next_slice;
  next_slice = Slice::getAllocated();
  
  // Adjust the end of this buffer
  this_slice->set_end( this_slice->end() + bytes_read );
  
  return this_slice;

}
