
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <cstring>
#include <stdio.h>
#include <picodrv.h>
#include <pico_errors.h>
#include <micronDAQ.h>
#include <system_error>


// add pad bytes to next multiple of 16 bytes
int micronDAQ::pad_for_16bytes(int len) {
	int pad_len = len;
	if(len%16 !=0) {
		pad_len = len + (16 - len%16); 
	}
	return pad_len;
}

// print <count> 256-bit numbers from buf
void micronDAQ::print256(FILE *f, void *buf, int count)
{
	uint32_t	*u32p = (uint32_t*) buf;

	for (int i=0; i < count; ++i){
	//	fprintf(f, "0x%08x_%08x_%08x_%08x_%08x_%08x_%08x_%08x\n", u32p[4*i+7], u32p[4*i+6], u32p[4*i+5],u32p[4*i+4], u32p[4*i+3], u32p[4*i+2], u32p[4*i+1], u32p[4*i]);
	unsigned int j = i+1;
	unsigned int k = i+2;

	fprintf(f, "0x%08x_%08x_%08x_%08x_%08x_%08x_%08x_%08x\n", u32p[4*i+7], u32p[4*i+6], u32p[4*i+5],u32p[4*j+4], u32p[4*j+3], u32p[4*j+2], u32p[4*j+1], u32p[4*k]);
}
}

// print <count> 128-bit numbers from buf
void micronDAQ::print128(FILE *f, void *buf, int count)
{
	uint32_t	*u32p = (uint32_t*) buf;

	for (int i=0; i < count; ++i)
		fprintf(f, "0x%08x_%08x_%08x_%08x\n", u32p[4*i+3], u32p[4*i+2], u32p[4*i+1], u32p[4*i]);
}

const size_t micronDAQ::getPacketBufferSize(){
	return packetBufferSize_;
}
const bool micronDAQ::getLoadBitFile(){
	return loadBitFile;
}

const std::string micronDAQ::getBitFileName(){
	return bitFileName;
}

int micronDAQ::runMicronDAQ(PicoDrv *pico, char **ibuf)
{
	int         err, i, j, stream1, stream2;
	uint32_t    *rbuf2, *rbuf, *wbuf, u32, addr;
	size_t	size;
//	char        ibuf[getPacketBufferSize()];
	//char        ibuf[1048576];
	//PicoDrv     *pico;
	int size_c = getBitFileName().length();
	char bitFileName_c[size_c + 1];

	// The RunBitFile function will locate a Pico card that can run the given bit file, and is not already
	//   opened in exclusive-access mode by another program. It requests exclusive access to the Pico card
	//   so no other programs will try to reuse the card and interfere with us.
	strcpy(bitFileName_c, getBitFileName().c_str());

	const char* bitFileName_const = bitFileName_c;
//	if(getLoadBitFile()){ 
	//	printf("Loading FPGA with '%s' ...\n", bitFileName_const);
	//	err = RunBitFile(bitFileName_const, &pico);
//	} else{
//		err = FindPico(0x852, &pico);
//	}   



	//	if (err < 0) {
	// We use the PicoErrors_FullError function to decipher error codes from RunBitFile.
	// This is more informative than just printing the numeric code, since it can report the name of a
	//   file that wasn't found, for example.
	//		fprintf(stderr, "%s: RunBitFile error: %s\n", argv[0], PicoErrors_FullError(err, ibuf, sizeof(ibuf)));
	//		exit(-1);
	//	}

	// Data goes out to the FPGA on WriteStream ID 1 and comes back to the host on ReadStream ID 1
	// the following function call (CreateStream) opens stream ID 1

	// Pico streams come in two flavors: 4Byte wide, 16Byte wide (equivalent to 32bit, 128bit respectively)
	// However, all calls to ReadStream and WriteStream must be 16Byte aligned (even for 4B wide streams)
	//
	// Now allocate 16B aligned space for read and write stream buffers
	//
	// Similarily, the size of the call, in bytes, must also be a multiple of 16Bytes. So check and pad
	// to next 16Byte multiple
	//size = 1048576 * sizeof(uint32_t);
	//size = 16384 * sizeof(uint32_t);
	size = getPacketBufferSize() * sizeof(uint32_t);
	size = pad_for_16bytes(size);

	if (malloc) {
	/*	err = posix_memalign((void**)&wbuf, 16, size);
		if (wbuf == NULL || err) {
			fprintf(stderr, "%s: posix_memalign could not allocate array of %zd bytes for write buffer\n", "bitfile", size);
			exit(-1);
		}*/
		err = posix_memalign((void**)&rbuf, 16, size);
		err = posix_memalign((void**)&rbuf2, 16, size);
		if (rbuf == NULL || err) {
			fprintf(stderr, "%s: posix_memalign could not allocate array of %zd bytes for read buffer\n", "bitfile", size);
			exit(-1);
		}
	} else {
		printf("%s: malloc failed\n", "bitfile");
		exit(-1);
	}


	// clear our read buffer to prepare for the read.

	// After we wrote some data to the FPGA, we expect the FPGA to operate upon that data and place
	// some results into an output stream.
	//
	// As with WriteStream, a ReadStream will block until all data is returned from the FPGA to the host.
	//
	// A user application will either have a deterministic amount of results, or a non-deterministic amount. 
	// - When a non-deterministic amount of results are expected, and given the blocking nature of the ReadStream,
	//   a user should use the GetBytesAvailable() call to determine the amount of data available for retrieval.
	// - When a deterministic amount of results is expected, a user can skip the performance impacting
	//   GetBytesAvailable() call and request the full amount of results. The user could also call ReadStream()
	//   iteratively, without GetBytesAvailable(), in which case getting smaller chunks of results may allow
	//   additional processing of the data on the host while the FPGA generates more results. 
	//   Either approach works, and should be kept in mind when tuning performance of your application.
	//
	// In the example below we use the GetBytesAvailable() call as an example. However since the StreamLoopback 
	// firmware returns exactly 1 piece of output data for every 1 piece of input data that it receives this
	// is not really necessary.
	//
	// All we have done thus far (after the call to WriteStream) is to clear out a buffer.
	// However, by calling GetBytesAvailable, we will see that the FPGA has already processed some of that 
	// data and placed it into an output stream.
	//i = pico->GetBytesAvailable(stream1_, true /* reading */);
	//j = pico->GetBytesAvailable(stream2, true /* reading */);
	/*	if (i < 0){
		fprintf(stderr, "%s: GetBytesAvailable error: %s\n", "bitfile", PicoErrors_FullError(i, ibuf, sizeof(ibuf)));
		exit(-1);
		}
		if (j < 0){
		fprintf(stderr, "%s: GetBytesAvailable error: %s\n", "bitfile", PicoErrors_FullError(j, ibuf, sizeof(ibuf)));
		exit(-1);
		}
		*/
	//printf("%d Bytes are available to read from the FPGA: stream 1 .\n", i);
	//printf("%d Bytes are available to read from the FPGA: stream 2 .\n", j);

	// Since the StreamLoopback firmware echoes 1 piece of data back to the software for every piece of data
	// that the software writes to it, we know that we can eventually read exactly the amount of data that
	// was written to the FPGA.

	// Here is where we actually call ReadStream
	// This reads "size" number of bytes of data from the output stream specified by our stream handle (e.g. 'stream') 
	// into our host buffer (rbuf)
	// This call will block until it is able to read the entire "size" Bytes of data.
	//size=pad_for_16bytes(pico->GetBytesAvailable(stream1, true)) - 16;

	memset(rbuf, 0, sizeof(rbuf));
	//memset(rbuf2, 0, sizeof(rbuf2));
	//printf("Reading stream 1 %zd B\n", i);
	//	while(1){
	err = pico->ReadStream(stream1_, rbuf, size);
	if (err < 0) {
		//fprintf(stderr, "%s: ReadStream error: %s\n", "bitfile", PicoErrors_FullError(err, **ibuf, getPacketBufferSize()));
		exit(-1);
	}

	//size=pad_for_16bytes(pico->GetBytesAvailable(stream2, true)) - 16;
	/*printf("Reading stream 2 %zd B\n", size);
	  err = pico->ReadStream(stream2, rbuf, size);
	  if (err < 0) {
	  fprintf(stderr, "%s: ReadStream error: %s\n", argv[0], PicoErrors_FullError(err, ibuf, sizeof(ibuf)));
	  exit(-1);
	  }*/

	// Now that we have received all of our read data back from the FPGA, we print out the first 
	// 512 bytes for the user.
	// In this sample, it makes the most sense to look at this read data 16B at a time, so we print out 
	// 16B chunks
	//printf("Data received back from FPGA:\n");
	print256(stdout, rbuf, getPacketBufferSize()/32);
	//std::cout << ibuf << std::endl;
	//	}
	std::cout << "debug1" << std::endl;
	uint32_t	*u32p = (uint32_t*) rbuf;

        for (int i=0; i < ((getPacketBufferSize()/32)+2); ++i){
        //      fprintf(f, "0x%08x_%08x_%08x_%08x_%08x_%08x_%08x_%08x\n", u32p[4*i+7], u32p[4*i+6], u32p[4*i+5],u32p[4*i+4], u32p[4*i+3], u32p[4*i+2], u32p[4*i+1], u32p[4*i]);
                unsigned int j = i+1;
                unsigned int k = i+2;
  //std::cout << u32p[4*i+7] << std::endl;      
		**ibuf << u32p[4*i+7] << u32p[4*i+6] << u32p[4*i+5] << u32p[4*j+4] << u32p[4*j+3] << u32p[4*j+2] << u32p[4*j+1] << u32p[4*k];    
}
	


	//free(wbuf);
	//free(rbuf);
	//return rbuf;
	return 1;
	//exit(0);
}


micronDAQ::micronDAQ( size_t packetBufferSize, size_t nbPacketBuffers, ctrl& control, config& conf ) :
	InputFilter( packetBufferSize, nbPacketBuffers, control ), bitFileName (conf.getBitFileName()), loadBitFile (conf.getLoadBitFile()), 
packetBufferSize_ (packetBufferSize) 
{
	// Initialize the DMA subsystem
	/*	if ( wz_init( &dma_ ) < 0 ) {
		std::string msg = "Cannot initialize WZ DMA device";
		if (errno == ENOENT) {
		msg += ". Check if xdma kernel module is loaded ('lsmod | grep xdma') and the board is visible on the PCIe bus ('lspci | grep Xilinx'). Error";
		}
		throw std::system_error(errno, std::system_category(), msg);
		}
		*/
	PicoDrv     *pico;
	int size_c = getBitFileName().length();
	char bitFileName_c[size_c + 1];
	//int stream1, stream2, err;
	int err;
	// The RunBitFile function will locate a Pico card that can run the given bit file, and is not already
	//   opened in exclusive-access mode by another program. It requests exclusive access to the Pico card
	//   so no other programs will try to reuse the card and interfere with us.
	strcpy(bitFileName_c, getBitFileName().c_str());

	const char* bitFileName_const = bitFileName_c;
	if(getLoadBitFile()){
		printf("Loading FPGA with '%s' ...\n", bitFileName_const);
		err = RunBitFile(bitFileName_const, &pico);
	} else{
		err = FindPico(0x852, &pico);
	}
	printf("Opening streams to and from the FPGA\n");
	stream1_ = pico->CreateStream(1);
//	stream2_ = pico->CreateStream(2);
	if (stream1_ < 0) {
		// All functions in the Pico API return an error code.  If that code is < 0, then you should use
		// the PicoErrors_FullError function to decode the error message.
		fprintf(stderr, "%s: CreateStream error: %s\n", "bitfile", PicoErrors_FullError(stream1_, 0, getPacketBufferSize()));
		//    fprintf(stderr, "%s: CreateStream error: %s\n", argv[0], PicoErrors_FullError(stream2, ibuf, sizeof(ibuf)));
		exit(-1);
	}
	
	setPicoDrv(pico);

	// Start the DMA
//	if ( runMicronDAQ() < 0) {
//		throw std::system_error(errno, std::system_category(), "Cannot start MicronDAQ");

//	}

	//	LOG(TRACE) << "Created MicronDAQ input filter";
}

void micronDAQ::setPicoDrv(PicoDrv* pico){
pico_ = pico; 
}

PicoDrv* micronDAQ::getPicoDrv(){
return pico_;
}

micronDAQ::~micronDAQ() {
	// streams are automatically closed when the PicoDrv object is destroyed, or on program termination, but
	//  we can also close a stream manually.
	pico_->CloseStream(stream1_);
	//pico_->CloseStream(stream2_);
//	LOG(TRACE) << "Closed pico streams";
//	LOG(TRACE) << "Destroyed micronDAQ input filter";
}


/**************************************************************************
 *  * Entry points are here
 *   * Overriding virtual functions
 *    */


//Print some additional info
void micronDAQ::print(std::ostream& out) const
{
	/*	out
		<< ", DMA errors " << stats.nbDmaErrors
		<< ", oversized " << stats.nbDmaOversizedPackets
		<< ", resets " << stats.nbBoardResets;
		*/
}


// Read a packet from DMA
ssize_t micronDAQ::readInput(char **buffer, size_t bufferSize)
{
	// We need at least 1MB buffer
	//	assert( bufferSize >= 1024*1024 );
	runMicronDAQ(getPicoDrv(), buffer);
	return getPacketBufferSize();
	//return WZDmaInputFilter::read_packet( buffer, bufferSize );
}


// Notifi the DMA that packet was processed
void micronDAQ::readComplete(char *buffer) {
		(void)(buffer);

	// Free the DMA buffer
//	if ( wz_read_complete( &dma_ ) < 0 ) {
//	throw std::system_error(errno, std::system_category(), "Cannot complete WZ DMA read");
//	}
	
}
