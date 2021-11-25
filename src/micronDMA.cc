
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <cstring>
#include <stdio.h>
#include <picodrv.h>
#include <pico_errors.h>
#include <micronDMA.h>
#include <system_error>
#include <boost/multiprecision/cpp_int.hpp>

using uint256_t  = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256,  256,  boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>, boost::multiprecision::et_off>;



// add pad bytes to next multiple of 16 bytes
int micronDMA::pad_for_16bytes(int len) {
	int pad_len = len;
	if(len%16 !=0) {
		pad_len = len + (16 - len%16); 
	}
	return pad_len;
}

// print <count> 256-bit numbers from buf
void micronDMA::print256(FILE *f, void *buf, int count)
{
	uint32_t	*u32p = (uint32_t*) buf;

	for (int i=0; i < count; ++i){

		fprintf(f, "0x%08x_%08x_%08x_%08x_%08x_%08x_%08x_%08x\n", u32p[8*i+7], u32p[8*i+6], u32p[8*i+5],u32p[8*i+4], u32p[8*i+3], u32p[8*i+2], u32p[8*i+1], u32p[8*i]);
	}
}


// print <count> 128-bit numbers from buf
void micronDMA::print128(FILE *f, void *buf, int count)
{
	uint32_t	*u32p = (uint32_t*) buf;

	for (int i=0; i < count; ++i)
		fprintf(f, "0x%08x_%08x_%08x_%08x\n", u32p[4*i+3], u32p[4*i+2], u32p[4*i+1], u32p[4*i]);
}

const size_t micronDMA::getPacketBufferSize(){
	return packetBufferSize_;
}
const bool micronDMA::getLoadBitFile(){
	return loadBitFile;
}

const std::string micronDMA::getBitFileName(){
	return bitFileName;
}

int micronDMA::runMicronDAQ(PicoDrv *pico, char **ibuf)
{
	int         err, i, j, stream1, stream2;
	uint32_t    *rbuf, *wbuf, u32, addr;
	size_t	size;

	// Usually Pico streams come in two flavors: 4Byte wide, 16Byte wide (equivalent to 32bit, 128bit respectively)
	// However, all calls to ReadStream and WriteStream must be 16Byte aligned (even for 4B wide streams)
	// There is also an 'undocumented' 32Byte wide (256 bit) stream 
	// We are using that stream here (and in the firmware)
	//
	// Now allocate 32B aligned space for read and write stream buffers
	//
	// Similarily, the size of the call, in bytes, must also be a multiple of 16Bytes.
	
	size = getPacketBufferSize();
	//One can pad for 16 Bytes with this function, but we don't want to pad so we don't use it here
	//size = pad_for_16bytes(size);
	if (malloc) {
		err = posix_memalign((void**)&rbuf, 32, size);
		if (rbuf == NULL || err) {
			fprintf(stderr, "%s: posix_memalign could not allocate array of %zd bytes for read buffer\n", "bitfile", size);
			exit(-1);
		}
	} else {
		printf("%s: malloc failed\n", "bitfile");
		exit(-1);
	}


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
	//
	// By calling GetBytesAvailable, one can see how much data is ready to read // not used for speed
	//i = pico->GetBytesAvailable(stream1_, true /* reading */);
	//	if (i < 0){
	//	fprintf(stderr, "%s: GetBytesAvailable error: %s\n", "bitfile", PicoErrors_FullError(i, ibuf, sizeof(ibuf)));
	//	exit(-1);
	//	}

	// Here is where we actually call ReadStream
	// This reads "size" number of bytes of data from the output stream specified by our stream handle (e.g. 'stream') 
	// into our host buffer (rbuf)
	// This call will block until it is able to read the entire "size" Bytes of data.
	err = pico->ReadStream(stream1_, rbuf, size);
	if (err < 0) {
		//fprintf(stderr, "%s: ReadStream error: %s\n", "bitfile", PicoErrors_FullError(err, **ibuf, getPacketBufferSize()));
		exit(-1);
	}


	uint32_t	*u32p = (uint32_t*) rbuf;

	memcpy(*ibuf, rbuf, size);

	free(rbuf);
	return 1;
}


micronDMA::micronDMA( size_t packetBufferSize, size_t nbPacketBuffers, ctrl& control, config& conf ) :
	InputFilter( packetBufferSize, nbPacketBuffers, control ), bitFileName (conf.getBitFileName()), loadBitFile (conf.getLoadBitFile()), 
	packetBufferSize_ (packetBufferSize) 
{
	PicoDrv     *pico;
	int size_c = getBitFileName().length();
	char bitFileName_c[size_c + 1];
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
	if (stream1_ < 0) {
		// All functions in the Pico API return an error code.  If that code is < 0, then you should use
		// the PicoErrors_FullError function to decode the error message.
		fprintf(stderr, "%s: CreateStream error: %s\n", "bitfile", PicoErrors_FullError(stream1_, 0, getPacketBufferSize()));
		exit(-1);
	}

	setPicoDrv(pico);

}

void micronDMA::setPicoDrv(PicoDrv* pico){
	pico_ = pico; 
}

PicoDrv* micronDMA::getPicoDrv(){
	return pico_;
}

micronDMA::~micronDMA() {
	// streams are automatically closed when the PicoDrv object is destroyed, or on program termination, but
	//  we can also close a stream manually.
	pico_->CloseStream(stream1_);
}

//Print some additional info
void micronDMA::print(std::ostream& out) const
{
}


// Read a packet from DMA
ssize_t micronDMA::readInput(char **buffer, size_t bufferSize)
{
	runMicronDAQ(getPicoDrv(), buffer);
	return getPacketBufferSize();
}


// Notifi the DMA that packet was processed
void micronDMA::readComplete(char *buffer) {
	(void)(buffer);

}
