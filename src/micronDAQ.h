#ifndef MICRONDAQ_H
#define MICRONDAQ_H

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
#include <cassert>
class micronDAQ : public InputFilter {
	public:
		micronDAQ();
		virtual ~micronDAQ();

		int pad_for_16bytes(int);
		void print128(FILE*, void*, int);
		int runMicronDAQ(int, char**);
	private:

	protected:
		ssize_t readInput(char **buffer, size_t bufferSize); // Override
		void readComplete(char *buffer);  // Override
		void print(std::ostream& out) const;  // Override
};
typedef std::shared_ptr<micronDAQ> micronDAQPtr;


#endif
