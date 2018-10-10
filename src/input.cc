#include <cstdio>
#include "input.h"
#include "slice.h"

InputFilter::InputFilter( FILE* input_file_ , size_t max_size_, 
			  size_t nslices_) : 

    filter(serial_in_order),
    input_file(input_file_),
    next_slice(Slice::preAllocate( max_size_,nslices_) ),
    counts(0)
{ 
    fprintf(stderr,"Created input filter and allocated at 0x%llx \n",(unsigned long long)next_slice);
}

InputFilter::~InputFilter() {
  fprintf(stderr,"Destroy input filter and delete at 0x%llx \n",(unsigned long long)next_slice);
  Slice::giveAllocated(next_slice);
  fprintf(stderr,"input operator total %u  read \n",counts);
}
 
void* InputFilter::operator()(void*) {

    size_t m = next_slice->avail();
    size_t n = fread( next_slice->begin(), 1, m, input_file );

    if(n<m){
      fprintf(stderr,"input operator read less %d \n",n);

    }
    counts+=n/192;

    if( n==0 ) {
      return NULL;
    } else {
        // Have more data to process.
        Slice& t = *next_slice;
        next_slice = Slice::getAllocated();

        char* p = t.end()+n;
        t.set_end(p);

        return &t;
    }

}
