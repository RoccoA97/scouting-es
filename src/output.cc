#include <system_error>
#include <iostream>
#include <fstream>

#include "output.h"
#include "slice.h"


OutputStream::OutputStream( const char* output_file_base, ctrl *c) : 
    tbb::filter(serial_in_order),
    my_output_file_base(output_file_base),
    totcounts(0),
    current_file_size(0),
    file_count(-1),
    control(c),
    current_file(0),
    current_run_number(0),
    journal_name(my_output_file_base + "/" + "index.journal")
{
  fprintf(stderr,"Created output filter at 0x%llx \n",(unsigned long long)this);  
}

static void update_journal(std::string journal_name, uint32_t run_number, uint32_t index)
{
  std::string new_journal_name = journal_name + ".new";

  // Create a new journal file
  std::ofstream journal (new_journal_name);
  if (journal.is_open()) {
    journal << run_number << "\n" << index << "\n";
    journal.close();
  } else {
    std::cerr << "WARNING: Unable to open journal file";
  }

  // Replace the old journal
  if (rename(new_journal_name.c_str(), journal_name.c_str()) < 0 ) {
    perror("Journal file move failed");
  }
}

static bool read_journal(std::string journal_name, uint32_t& run_number, uint32_t& index)
{
    std::ifstream journal (journal_name);
    if (journal.is_open()) {
      journal >> run_number >> index;
      journal.close();
      return true;
    } 
    return false;
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
      current_run_number = 0;
    }
    out.free();
    return NULL;
}

void OutputStream::open_next_file(){

  // Close and move open file
  if (current_file) {
    fclose(current_file);

    // Move the file, so it can be processes by file move daemon
    char source_file_name[256];
    char target_file_name[256];
    // TODO: Check if the destination directory exists
    sprintf(source_file_name,"%s/in_progress/scout_%06d_%06d.dat",my_output_file_base.c_str(),current_run_number,file_count);
    sprintf(target_file_name,"%s/scout_%06d_%06d.dat",my_output_file_base.c_str(),current_run_number,file_count);

    fprintf(stderr, "rename: %s to %s\n", source_file_name, target_file_name);
    if ( rename(source_file_name, target_file_name) < 0 ) {
      perror("File rename failed");
    }

    current_file_size=0; 
    file_count += 1;
  }

  // We can change current_run_number only here, after the (previous) file was closed and moved
  current_run_number = control->run_number;

  // If this is the first file check if we have a journal file
  if (file_count < 0) {
    // Set default file index
    file_count = 0;

    uint32_t run_number;
    uint32_t index;

    if (read_journal(journal_name, run_number, index)) {
      std::cout << "We have journal:\n";
      std::cout << "  run_number: " << run_number << '\n';
      std::cout << "  index:      " << index << "\n";   
    } else {
      std::cout << "No journal file.\n";
    }

    std::cout << "Current run_number: " << current_run_number << '\n';
    if (current_run_number == run_number) {
      file_count = index;
    }
    std::cout << "  using index:      " << file_count << '\n';    
  }

  // Create a new file
  char run_order_stem[256];
  // TODO: Check if the destination directory exists
  sprintf(run_order_stem,"/in_progress/scout_%06d_%06d.dat",current_run_number,file_count);
  std::string file_end(run_order_stem);
  std::string filename = my_output_file_base+file_end;
  current_file = fopen( filename.c_str(), "w" );
  if (current_file == NULL) {
    throw std::system_error(errno, std::system_category(), "File open error");
  }

  // Update journal file (with the next index file)
  update_journal(journal_name, current_run_number, file_count+1);
}
