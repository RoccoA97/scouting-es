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
		virtual ~MicronDmaInputFilter();

	protected:
		ssize_t readInput(char **buffer, size_t bufferSize); // Override
		void print(std::ostream& out) const;  // Override

	private:
		PicoDrv* pico_;
		int stream1_;

		ssize_t  runMicronDAQ(char **buffer, size_t bufferSize);
};
typedef std::shared_ptr<MicronDmaInputFilter> MicronDmaInputFilterPtr;

#endif // MICRONDMA_INPUT_FILER_H
