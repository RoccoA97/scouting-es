#include "output.h"
#include "slice.h"


OutputStream::OutputStream( const char* output_file_base, ctrl *c) : 
    tbb::filter(serial_in_order),
    my_output_file_base(output_file_base),
    totcounts(0),
    current_file_size(0),
    file_count(0),
    control(c),
    current_file(0)
{
  fprintf(stderr,"Created output filter at 0x%llx \n",(unsigned long long)this);  
}

void* OutputStream::operator()( void* item ) {
    Slice& out = *static_cast<Slice*>(item);
    totcounts += out.get_counts();
    if(control->running){
      if(current_file==0 || current_file_size > control->max_file_size) open_next_file();
      
      size_t n = fwrite( out.begin(), 1, out.size(), current_file );
      current_file_size+=n;      
      if( n!=out.size() ) {
        fprintf(stderr,"Can't write into output file \n");
      }
    }
    if(!control->running && current_file!=0){
      fclose(current_file);
      current_file=0;
      file_count = 0;
    }
    out.free();
    return NULL;
}

void OutputStream::open_next_file(){
  if(current_file){fclose(current_file); current_file_size=0; file_count += 1;}
  char run_order_stem[18];
  sprintf(run_order_stem,"_%06d_%06d.dat",control->run_number,file_count);
  std::string file_end(run_order_stem);
  std::string filename = my_output_file_base+file_end;
  current_file = fopen( filename.c_str(), "w" );
}
