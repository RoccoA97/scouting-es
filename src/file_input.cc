#include <cstdio>
#include "file_input.h"
#include "slice.h"
#include "utility.h"

FileInputFilter::FileInputFilter( const std::string& file_name_, size_t max_size_, 
			  size_t nslices_) : 

  filter(serial_in_order),
  next_slice(Slice::preAllocate( max_size_,nslices_) ),
  counts(0),
  ncalls(0),
  lastStartTime(tbb::tick_count::now()),
  last_count(0)
{ 
  input_file = fopen( file_name_.c_str(), "r" );
  if ( !input_file ) {
    throw std::invalid_argument( ("Invalid input file name: " + file_name_).c_str() );
  }
  fprintf(stderr,"Created input filter and allocated at 0x%llx \n",(unsigned long long)next_slice);
}

FileInputFilter::~FileInputFilter() {
  fprintf(stderr,"Destroy input filter and delete at 0x%llx \n",(unsigned long long)next_slice);
  Slice::giveAllocated(next_slice);
  fprintf(stderr,"input operator total %lu  read \n",counts);
  //  fclose( output_file );
  fclose( input_file );
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
    return this->operator()(nullptr);
  } else {
    // Have more data to process.
    Slice& t = *next_slice;
    next_slice = Slice::getAllocated();
    
    char* p = t.end()+n;
    t.set_end(p);
    
    return &t;
  }
  
}
