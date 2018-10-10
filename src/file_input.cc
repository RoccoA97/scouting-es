#include <cstdio>
#include "file_input.h"
#include "slice.h"
#include "utility.h"

FileInputFilter::FileInputFilter( FILE* input_file_ , size_t max_size_, 
			  size_t nslices_) : 

  filter(serial_in_order),
  input_file(input_file_),
  next_slice(Slice::preAllocate( max_size_,nslices_) ),
  counts(0),
  ncalls(0),
  lastStartTime(tbb::tick_count::now()),
  last_count(0)
{ 
  fprintf(stderr,"Created input filter and allocated at 0x%llx \n",(unsigned long long)next_slice);
}

FileInputFilter::~FileInputFilter() {
  fprintf(stderr,"Destroy input filter and delete at 0x%llx \n",(unsigned long long)next_slice);
  Slice::giveAllocated(next_slice);
  fprintf(stderr,"input operator total %u  read \n",counts);
}

void* FileInputFilter::operator()(void*) {
  
  ncalls++;
  size_t m = next_slice->avail();
  size_t n = fread( next_slice->begin(), 1, m, input_file );
  
  if(n<m){
    //      fprintf(stderr,"input operator read less %d \n",n);
    
  }
  counts+=n/192;
  
  if(ncalls%100000==0){
    printf("input bandwidth %f\n",double((counts-last_count)*192/1024./1024.)/
	   double((tbb::tick_count::now() - lastStartTime).seconds()));
    lastStartTime=tbb::tick_count::now();
    last_count=counts;
  } 
  if( n==0 ) {
    fseek(input_file,0,SEEK_SET);
    void *a;
    return this->operator()(a);
  } else {
    // Have more data to process.
    Slice& t = *next_slice;
    next_slice = Slice::getAllocated();
    
    char* p = t.end()+n;
    t.set_end(p);
    
    return &t;
  }
  
}
