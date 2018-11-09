#include <cassert>
#include <cerrno>
#include <system_error>
#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "FileDmaInputFilter.h"
#include "log.h"


FileDmaInputFilter::FileDmaInputFilter( const std::string& fileName, size_t packetBufferSize, size_t nbPacketBuffers, ctrl& control ) : 
  InputFilter( packetBufferSize, nbPacketBuffers, control )
{ 
  inputFile = fopen( fileName.c_str(), "r" );
  if ( !inputFile ) {
    throw std::invalid_argument( "Invalid input file name: " + fileName );
  }
  LOG(TRACE) << "Created file input filter"; 
}

FileDmaInputFilter::~FileDmaInputFilter() {
  fclose( inputFile );
  LOG(TRACE) << "Destroyed file input filter";
}

/*
 * This function reads packet by packet from a file
 * in order to simulate DMA reads.
 * 
 * TODO: Better would be mmap directly the file
 */ 
static inline ssize_t read_dma_packet_from_file(FILE *inputFile, char *buffer, uint64_t size, uint64_t nbReads)
{
  static constexpr uint64_t deadbeaf = 0xdeadbeefdeadbeefL;
  size_t bytesRead = 0;
  size_t rc;

  while ( !feof(inputFile) && bytesRead < size) {
    // Expecting 32 byte alignment 
    rc = fread( buffer, 1, 32, inputFile );

    if (ferror(inputFile)) {
        throw std::system_error(errno, std::system_category(), "File error");
    }

    if (rc != 32) {
      if ( feof(inputFile) && rc == 0 && bytesRead == 0) {
        // We have reached the perfect end of the file, let's start again
        fseek(inputFile, 0, SEEK_SET);
        continue;
      }

      // Misaligned data
      throw std::runtime_error("#" + std::to_string(nbReads) + ": File read ends prematurely, missing " + std::to_string(32-rc) + " bytes. Something is probably misaligned.");
    }

    bytesRead += 32;

    if ( *(uint64_t*)(buffer) == deadbeaf ) {
      // End of the packet was found
      return bytesRead;
    }

    buffer += 32;
  }

  // We are here either
  //   because we found EOF earlier than the end of the packet 
  //   or the packet cannot fit into the buffer
  // We can deal with both conditions but for the moment we throw an error 

  if (feof(inputFile)) {
    throw std::runtime_error("EOF reached but no end of the packet found.");
  }

  throw std::runtime_error("Packet is too big and cannot fit into preallocated memory");
  // TODO: Read and discard data until deadbeef is found

  return -1;
}


inline ssize_t FileDmaInputFilter::readPacket(char **buffer, size_t bufferSize)
{
  // Read from DMA
  int skip = 0;
  ssize_t bytesRead = read_dma_packet_from_file(inputFile, *buffer, bufferSize, nbReads() );

  // If large packet returned, skip and read again
  while ( bytesRead > (ssize_t)bufferSize ) {
    stats.nbOversizedPackets++;
    skip++;
    LOG(ERROR)  
      << "#" << nbReads() << ": ERROR: Read returned " << bytesRead << " > buffer size " << bufferSize
      << ". Skipping packet #" << skip << ".";
    if (skip >= 100) {
      throw std::runtime_error("FATAL: Read is still returning large packets.");
    }
    bytesRead = read_dma_packet_from_file(inputFile, *buffer, bufferSize, nbReads() );
  }

  return bytesRead;
}


/**************************************************************************
 * Entry points are here
 * Overriding virtual functions
 */

// Print some additional info
void  FileDmaInputFilter::print(std::ostream& out) const
{
  out << ", oversized packets " << stats.nbOversizedPackets;
}

ssize_t FileDmaInputFilter::readInput(char **buffer, size_t bufferSize)
{
  return readPacket( buffer, bufferSize );
}


