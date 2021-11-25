#ifndef MICRONDMA_INPUT_FILER_H
#define MICRONDMA_INPUT_FILER_H

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pico_drv.h>
#include <pico_errors.h>
#include <stdint.h>
#include <memory>
#include <InputFilter.h>
#include <WZDmaInputFilter.h>
#include <config.h>
#include <cassert>

class MicronDmaInputFilter : public InputFilter {
	public:
  		MicronDmaInputFilter( size_t, size_t , ctrl&, config& );
		MicronDmaInputFilter();
		virtual ~MicronDmaInputFilter();

		int pad_for_16bytes(int);
		void print128(FILE*, void*, int);
		void print256(FILE*, void*, int);
		int runMicronDAQ(PicoDrv*, char**);
		const bool getLoadBitFile();
		const std::string getBitFileName();
		const size_t getPacketBufferSize();
		PicoDrv* getPicoDrv();
	private:
        const std::string bitFileName;
        const bool loadBitFile;
        const size_t packetBufferSize_;
	PicoDrv* pico_;
	int stream1_, stream2_;
	void setPicoDrv(PicoDrv*);

	protected:
		ssize_t readInput(char **buffer, size_t bufferSize); // Override
		void readComplete(char *buffer);  // Override
		void print(std::ostream& out) const;  // Override
};
typedef std::shared_ptr<MicronDmaInputFilter> MicronDmaInputFilterPtr;

#endif // MICRONDMA_INPUT_FILER_H
