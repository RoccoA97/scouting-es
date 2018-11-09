#include <cassert>
#include <cerrno>
#include <system_error>
#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "tools.h"
#include "WZDmaInputFilter.h"
#include "log.h"


WZDmaInputFilter::WZDmaInputFilter( size_t packetBufferSize, size_t nbPacketBuffers, ctrl& control ) : 
  InputFilter( packetBufferSize, nbPacketBuffers, control )
{ 
  // Initialize the DMA subsystem 
	if ( wz_init( &dma_ ) < 0 ) {
    std::string msg = "Cannot initialize WZ DMA device";
    if (errno == ENOENT) {
      msg += ". Check if xdma kernel module is loaded ('lsmod | grep xdma') and the board is visible on the PCIe bus ('lspci | grep Xilinx'). Error";
    }
    throw std::system_error(errno, std::system_category(), msg);
  }

	// Start the DMA
	if ( wz_start_dma( &dma_ ) < 0) {
    throw std::system_error(errno, std::system_category(), "Cannot start WZ DMA");
	}

  LOG(TRACE) << "Created WZ DMA input filter"; 
}

WZDmaInputFilter::~WZDmaInputFilter() {
  wz_stop_dma( &dma_ );
	wz_close( &dma_ );
  LOG(TRACE) << "Destroyed WZ DMA input filter";
}


inline ssize_t WZDmaInputFilter::read_packet_from_dma(char **buffer)
{
  int tries = 1;
  ssize_t bytes_read;

  while (1) {
    bytes_read = wz_read_start( &dma_, buffer );

    if (bytes_read < 0) {
      stats.nbDmaErrors++;
      tools::perror("#" + std::to_string( nbReads() ) + ": Read failed, returned: " + std::to_string(bytes_read));

      if (errno == EIO) {
        std::cerr << "#" << nbReads() << ": Trying to restart DMA (attempt #" << tries << "): ";
        wz_stop_dma( &dma_ );
        wz_close( &dma_ );

        // Initialize the DMA subsystem 
        if ( wz_init( &dma_ ) < 0 ) {
          throw std::system_error(errno, std::system_category(), "Cannot initialize WZ DMA device");
        }

        if (wz_start_dma( &dma_ ) < 0) {
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


inline ssize_t WZDmaInputFilter::read_packet( char **buffer, size_t bufferSize )
{
  ssize_t bytesRead;

  // Read from DMA
  int skip = 0;
  int reset = 0;
  bytesRead = read_packet_from_dma( buffer );

  // If large packet returned, skip and read again
  while ( bytesRead > (ssize_t)bufferSize ) {
    stats.nbDmaOversizedPackets++;
    skip++;
    std::cerr 
      << "#" << nbReads() << ": ERROR: DMA read returned " << bytesRead << " > buffer size " << bufferSize
      << ". Skipping packet #" << skip << ".\n";
    if (skip > 100) {
      reset++;
      stats.nbBoardResets++;

      if (reset > 10) {
        std::cerr << "Resets didn't help!\n";
        throw std::runtime_error("FATAL: DMA is still returning large packets.");
      }

      // Oversized packet is usually sign of link problem
      // Let's try to reset the board
      std::cerr << "Goging to reset the board: \n";
      if (wz_reset_board() < 0) {
        std::cerr << "Reset finished\n";
      } else {
        std::cerr << "Reset succesfull\n";
      }

      std::cerr << "Waiting for 30 seconds to clear any collected crap: ";
      // Sleep for 30 seconds (TCDS may be paused)
      for (int i=0; i<30; i++) {
        std::cout << '.' << std::flush;
        usleep(1000000);
      }
      std::cerr << " OK\n";
    }
    bytesRead = read_packet_from_dma( buffer );
  }

  return bytesRead;
}


/**************************************************************************
 * Entry points are here
 * Overriding virtual functions
 */


// Print some additional info
void WZDmaInputFilter::print(std::ostream& out) const
{
    out 
      << ", DMA errors " << stats.nbDmaErrors
      << ", DMA oversized packets " << stats.nbDmaOversizedPackets
      << ", board resets " << stats.nbBoardResets;
}


// Read a packet from DMA
ssize_t WZDmaInputFilter::readInput(char **buffer, size_t bufferSize)
{
  // We need at least 1MB buffer
  assert( bufferSize >= 1024*1024 );

  return read_packet( buffer, bufferSize );
}


// Notifi the DMA that packet was processed
void WZDmaInputFilter::readComplete(char *buffer) {
  (void)(buffer);

  // Free the DMA buffer
  if ( wz_read_complete( &dma_ ) < 0 ) {
      throw std::system_error(errno, std::system_category(), "Cannot complete WZ DMA read");
	}  
}