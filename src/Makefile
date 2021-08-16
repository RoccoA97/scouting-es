# example makefile
#
# Notes:
# - the indents must be tabs
# - if you need to specify a non-standard compiler, then set CXX
#   (it defaults to g++)
# - if you need to pass flags to the linker, set LDFLAGS rather
#   than alter the ${TARGET}: ${OBJECTS} rule
# - to see make's built-in rules use make -p
# target executable name
TARGET = scdaq
CXXFLAGS = -std=c++11 -Wall -Wextra -g -rdynamic -O3 -Wno-multichar -DLINUX -DPOSIX -I. -I${PICOBASE}/software/include/linux -I${PICOBASE}/software/include -fmessage-length=0 -fdiagnostics-show-option -fms-extensions -Wno-write-strings -DOSNAME="Linux" 
#-o micronDAQ micronDAQ.h ${PICOBASE}/software/source/GString.cpp ${PICOBASE}/software/source/pico_drv.cpp ${PICOBASE}/software/source/pico_errors.cpp ${PICOBASE}/software/source/linux/linux.cpp ${PICOBASE}/software/source/pico_drv_linux.cpp
#USER_SOURCES = micronDAQ.cc
#include $(PICOBASE)/software/Makefile.common
# source files
SOURCES = micronDAQ.cc config.cc dma_input.cc elastico.cc FileDmaInputFilter.cc file_input.cc  InputFilter.cc output.cc  processor.cc  scdaq.cc  session.cc  slice.cc WZDmaInputFilter.cc
C_SOURCES = wz_dma.c

# work out names of object files from sources
OBJECTS = $(SOURCES:.cc=.o)
OBJECTS += $(C_SOURCES:.c=.o)

# compiler flags (do not include -c here as it's dealt with by the
# appropriate rules; CXXFLAGS gets passed as part of command
# invocation for both compilation (where -c is needed) and linking
# (where it's not.)
#CXXFLAGS = -std=c++11 -Wall -Wextra -O0 -g -rdynamic
#CXXFLAGS = -std=c++11 -Wall -Wextra -g -rdynamic

CFLAGS = $(CXXFLAGS)
LDFLAGS = -ltbb -lboost_thread -lcurl

CPPFLAGS = -I. -Iwzdma -I${PICOBASE}/software/include -I${PICOBASE}/software/include/linux -I${PICOBASE}/software/source -DLINUX -DPOSIX

# default target (to build all)
all: ${TARGET}

# clean target
clean:
	rm ${OBJECTS} ${TARGET}

# rule to link object files to create target executable
# $@ is the target, here $(TARGET), and $^ is all the
# dependencies separated by spaces (duplicates removed),
# here ${OBJECTS}
${TARGET}: ${OBJECTS}
	${LINK.cc} -o $@ $^

# no rule is needed here for compilation as make already
# knows how to do it

# header dependencies (in more complicated cases, you can
# look into generating these automatically.)

#product.o : product.h test1.h test2.h

#test1.o : product.h test1.h test2.h

#test2.o : product.h test2.h

scdaq.o:	file_input.h processor.h elastico.h output.h format.h server.h controls.h config.h session.h log.h 
micronDAQ.o: micronDAQ.h ${PICOBASE}/software/source/GString.cpp ${PICOBASE}/software/include/pico_drv.h ${PICOBASE}/software/include/pico_errors.h ${PICOBASE}/software/include/linux/linux.h ${PICOBASE}/software/include/pico_drv_linux.h
config.o:	config.h log.h
dma_input.o:	dma_input.h slice.h
elastico.o:	elastico.h format.h slice.h controls.h log.h
file_dma_input.o:	file_dma_input.h
FileDmaInputFilter.o:	FileDmaInputFilter.h InputFilter.h log.h
file_input.o:	file_input.h slice.h utility.h
InputFilter.o:	InputFilter.h log.h
output.o:	output.h slice.h log.h
processor.o:	processor.h slice.h format.h log.h
session.o:	session.h log.h
slice.o: 	slice.h
WZDmaInputFilter.o:	WZDmaInputFilter.h InputFilter.h tools.h log.h
wz_dma.o:	wz_dma.h
