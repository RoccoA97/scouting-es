#include <cassert>
#include <iomanip>
#include <system_error>

#include "InputFilter.h"
#include "slice.h"
#include "controls.h"
#include "log.h"

InputFilter::InputFilter(size_t packetBufferSize, size_t nbPacketBuffers, ctrl& control) : 
    filter(serial_in_order),
    control_(control),
    nextSlice_(Slice::preAllocate( packetBufferSize, nbPacketBuffers )),
    nbReads_(0),
    nbBytesRead_(0),
    previousNbBytesRead_(0),
    previousStartTime_( tbb::tick_count::now() )
{ 
    minBytesRead_ = SSIZE_MAX;
    maxBytesRead_ = 0;
    previousNbReads_ = 0;
    LOG(TRACE) << "Created input filter and allocated at " << static_cast<void*>(nextSlice_);
}

InputFilter::~InputFilter() {
  LOG(TRACE) << "Destroy input filter and delete at " << static_cast<void*>(nextSlice_);

  Slice::giveAllocated(nextSlice_);
  LOG(DEBUG) << "Input operator performed " << nbReads_ << " read";
}


void InputFilter::printStats(std::ostream& out)
{
     // Calculate DMA bandwidth
    tbb::tick_count now = tbb::tick_count::now();
    double time_diff = (double)((now - previousStartTime_).seconds());
    previousStartTime_ = now;

    uint64_t nbReadsDiff = nbReads_ - previousNbReads_;
    previousNbReads_ = nbReads_;
    assert( nbReadsDiff > 0 );

    uint64_t nbBytesReadDiff = nbBytesRead_ - previousNbBytesRead_;
    previousNbBytesRead_ = nbBytesRead_;

    // Calculate bandwidth
    double bwd = nbBytesReadDiff / ( time_diff * 1024.0 * 1024.0 );

    // Calculate avg packet size
    uint64_t avgBytesRead = nbBytesReadDiff / nbReadsDiff;

	  // Save formatting
    std::ios state(nullptr);
	  state.copyfmt(out);

    out 
      << "#" << nbReads_ << ": Reading " << std::fixed << std::setprecision(1) << bwd << " MB/sec, " 
      << nbReadsDiff << " packet(s) min/avg/max " << minBytesRead_ <<  '/' << avgBytesRead << '/' << maxBytesRead_;
      
	  // Restore formatting
	  out.copyfmt(state);

    // Print additional info
    print( out );

    // Clear statistics
    minBytesRead_ = SSIZE_MAX;
    maxBytesRead_ = 0;     
}


void* InputFilter::operator()(void*) {
  // Prepare destination buffer
  char *buffer = nextSlice_->begin();
  // Available buffer size
  size_t bufferSize = nextSlice_->avail();
  ssize_t bytesRead = 0;

  nbReads_++;

  // It is optional to use the provided buffer
  bytesRead = readInput( &buffer, bufferSize );

  // This should really not happen
  assert( bytesRead != 0);

  if (buffer != nextSlice_->begin()) {
    // If read returned a different buffer, then it didn't use our buffer and we have to copy data
    // FIXME: It is a bit stupid to copy buffer, better would be to use zero copy approach 
    memcpy( nextSlice_->begin(), buffer, bytesRead );
  }

  // Notify that we processed the given buffer
  readComplete(buffer);

  // Update some stats
  nbBytesRead_ += bytesRead;

  // Update min/max
  minBytesRead_ = bytesRead < minBytesRead_ ? bytesRead : minBytesRead_;
  maxBytesRead_ = bytesRead > maxBytesRead_ ? bytesRead : maxBytesRead_;

  // Print some statistics
  if (control_.packets_per_report && (nbReads_ % control_.packets_per_report == 0)) {
    std::ostringstream log;
    printStats( log );
    LOG(INFO) << log.str();
  }

  // Have more data to process.
  Slice* thisSlice = nextSlice_;
  nextSlice_ = Slice::getAllocated();
  
  // Adjust the end of this buffer
  thisSlice->set_end( thisSlice->end() + bytesRead );
  
  return thisSlice;

}

